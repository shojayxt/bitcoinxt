// Copyright (c) 2015-2016 The Bitcoin XT developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "thinblock.h"
#include "thinblockmanager.h"
#include "primitives/block.h"
#include "blockencodings.h" // GetShortID
#include "util.h"
#include <sstream>


ThinTx::ThinTx(const uint256& h) : full_(h), cheap_(h.GetCheapHash())
{
    obfuscated_.id = 0;
}

ThinTx::ThinTx(const uint64_t& h) : cheap_(h)
{
    obfuscated_.id = 0;
}

ThinTx::ThinTx(const uint64_t& id,
        const uint64_t& idk0, const uint64_t& idk1) : cheap_(0) {
    obfuscated_.id = id;
    obfuscated_.idk0 = idk0;
    obfuscated_.idk1 = idk1;
}

void ThinTx::merge(const ThinTx& tx) {
    if (hasFull())
        return;

    if (tx.hasFull()) {
        full_ = tx.full();
        cheap_ = tx.cheap();
        return;
    }

    if (!hasCheap() && tx.hasCheap())
        cheap_ = tx.cheap();

    if (hasObfuscated())
        return;

    if (!tx.hasObfuscated())
        return;

    obfuscated_ = tx.obfuscated_;
}

bool ThinTx::hasFull() const {
    return !full_.IsNull();
}

const uint256& ThinTx::full() const {
    if (!hasFull())
        throw std::runtime_error("full hash not available");
    return full_;
}

const uint64_t& ThinTx::cheap() const {
    if (cheap_ == 0)
        throw std::runtime_error("cheap hash not available");

    return cheap_;
}

uint64_t ThinTx::obfuscated() const {
    if (obfuscated_.id == 0)
        throw std::runtime_error("obfuscated hash not available");
    return obfuscated_.id;
}

bool ThinTx::hasObfuscated() const {
    return obfuscated_.id != 0;
}

bool ThinTx::equals(const ThinTx& b) const {

    const bool indeterminate = false; //< can't know if txs equal or not

    if (isNull() && b.isNull())
        return true;

    if (isNull() || b.isNull())
        return false;

    if (hasFull() && b.hasFull())
        return full() == b.full();

    if (hasCheap() && b.hasCheap())
        return cheap() == b.cheap();

    if (hasObfuscated() && b.hasFull())
        return obfuscated_.id == GetShortID(
                obfuscated_.idk0, obfuscated_.idk1, b.full());

    if (hasFull() && b.hasObfuscated())
        return b.obfuscated_.id == GetShortID(
                b.obfuscated_.idk0, b.obfuscated_.idk1, full());

    if (hasObfuscated() && b.hasObfuscated()) {

        if (obfuscated_.idk0 != b.obfuscated_.idk0)
            return indeterminate;

        if (obfuscated_.idk1 != b.obfuscated_.idk1)
            return indeterminate;

        return obfuscated_.id == b.obfuscated_.id;
    }

    if (hasObfuscated() || b.hasObfuscated())
        return indeterminate;

    assert(!"ThinTx::equals");
}

bool ThinTx::equals(const uint256& b) const {
    return equals(ThinTx(b));
}

ThinBlockWorker::ThinBlockWorker(ThinBlockManager& m, NodeId n) :
    mg(m), rerequesting(false), node(n)
{
}

ThinBlockWorker::~ThinBlockWorker() {
    mg.delWorker(*this, node);
}

void ThinBlockWorker::buildStub(const StubData& d, const TxFinder& f) {
    assert(d.header().GetHash() == block);
    mg.buildStub(d, f);
}

bool ThinBlockWorker::isStubBuilt() const {
    return mg.isStubBuilt(block);
}

bool ThinBlockWorker::addTx(const CTransaction& tx) {
    return mg.addTx(block, tx);
}

std::vector<ThinTx> ThinBlockWorker::getTxsMissing() const {
    return mg.getTxsMissing(block);
}

void ThinBlockWorker::setAvailable() {
    if (isAvailable())
        return;
    mg.delWorker(*this, node);
    block.SetNull();
    rerequesting = false;
}

bool ThinBlockWorker::isAvailable() const {
    return block.IsNull();
}

void ThinBlockWorker::setToWork(const uint256& newblock) {
    assert(!newblock.IsNull());
    if (newblock == block)
        return;

    mg.delWorker(*this, node);
    block = newblock;
    rerequesting = false;
    mg.addWorker(newblock, *this);
}

bool ThinBlockWorker::isOnlyWorker() const {
    return mg.numWorkers(block) <= 1;
}

bool ThinBlockWorker::isReRequesting() const {
    return rerequesting;
}

void ThinBlockWorker::setReRequesting(bool r) {
    rerequesting = r;
}

uint256 ThinBlockWorker::blockHash() const {
    return block;
}

std::string ThinBlockWorker::blockStr() const {
    return block.ToString();
}

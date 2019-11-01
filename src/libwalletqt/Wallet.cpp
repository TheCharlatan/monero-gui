// Copyright (c) 2014-2019, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Wallet.h"
#include "PendingTransaction.h"
#include "UnsignedTransaction.h"
#include "TransactionHistory.h"
#include "AddressBook.h"
#include "Subaddress.h"
#include "SubaddressAccount.h"
#include "model/TransactionHistoryModel.h"
#include "model/TransactionHistorySortFilterModel.h"
#include "model/AddressBookModel.h"
#include "model/SubaddressModel.h"
#include "model/SubaddressAccountModel.h"
#include "wallet/api/wallet2_api.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QList>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>

namespace {
    static const int DAEMON_BLOCKCHAIN_HEIGHT_CACHE_TTL_SECONDS = 5;
    static const int DAEMON_BLOCKCHAIN_TARGET_HEIGHT_CACHE_TTL_SECONDS = 30;
    static const int WALLET_CONNECTION_STATUS_CACHE_TTL_SECONDS = 5;
}

class WalletListenerImpl : public  Monero::WalletListener
{
public:
    WalletListenerImpl(Wallet * w)
        : m_wallet(w)
    {

    }

    virtual void moneySpent(const std::string &txId, uint64_t amount)
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->moneySpent(QString::fromStdString(txId), amount);
    }


    virtual void moneyReceived(const std::string &txId, uint64_t amount)
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->moneyReceived(QString::fromStdString(txId), amount);
    }

    virtual void unconfirmedMoneyReceived(const std::string &txId, uint64_t amount)
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->unconfirmedMoneyReceived(QString::fromStdString(txId), amount);
    }

    virtual void newBlock(uint64_t height)
    {
        // qDebug() << __FUNCTION__;
        emit m_wallet->newBlock(height, m_wallet->daemonBlockChainTargetHeight());
    }

    virtual void updated()
    {
        emit m_wallet->updated();
    }

    // called when wallet refreshed by background thread or explicitly
    virtual void refreshed()
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->refreshed();
    }

    virtual void onDeviceButtonRequest(uint64_t code) override
    {
        emit m_wallet->deviceButtonRequest(code);
    }

    virtual void onDeviceButtonPressed() override
    {
        emit m_wallet->deviceButtonPressed();
    }

private:
    Wallet * m_wallet;
};

Wallet::Wallet(QObject * parent)
    : Wallet(nullptr, parent)
{
}

QString Wallet::getSeed() const
{
qCritical() << "QString Wallet::getSeed";
    return QString::fromStdString(m_walletImpl->seed());
}

QString Wallet::getSeedLanguage() const
{
qCritical() << "QString Wallet::getSeedLanguage";
    return QString::fromStdString(m_walletImpl->getSeedLanguage());
}

void Wallet::setSeedLanguage(const QString &lang)
{
qCritical() << "void Wallet::setSeedLanguage";
    m_walletImpl->setSeedLanguage(lang.toStdString());
}

Wallet::Status Wallet::status() const
{
qCritical() << "Status Wallet::status";
    return static_cast<Status>(m_walletImpl->status());
}

NetworkType::Type Wallet::nettype() const
{
qCritical() << "Type Wallet::nettype";
    return static_cast<NetworkType::Type>(m_walletImpl->nettype());
}


void Wallet::updateConnectionStatusAsync()
{
qCritical() << "void Wallet::updateConnectionStatusAsync";
    m_scheduler.run([this] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        ConnectionStatus newStatus = static_cast<ConnectionStatus>(m_walletImpl->connected());
        if (newStatus != m_connectionStatus || !m_initialized) {
            m_initialized = true;
            m_connectionStatus = newStatus;
            qDebug() << "NEW STATUS " << newStatus;
            emit connectionStatusChanged(newStatus);
        }
        // Release lock
        m_connectionStatusRunning = false;
    });
}

Wallet::ConnectionStatus Wallet::connected(bool forceCheck)
{
qCritical() << "ConnectionStatus Wallet::connected";
    // cache connection status
    if (forceCheck || !m_initialized || (m_connectionStatusTime.elapsed() / 1000 > m_connectionStatusTtl && !m_connectionStatusRunning) || m_connectionStatusTime.elapsed() > 30000) {
        qDebug() << "Checking connection status";
        m_connectionStatusRunning = true;
        m_connectionStatusTime.restart();
        updateConnectionStatusAsync();
    }

    return m_connectionStatus;
}

bool Wallet::synchronized() const
{
qCritical() << "bool Wallet::synchronized";
    return m_walletImpl->synchronized();
}

QString Wallet::errorString() const
{
qCritical() << "QString Wallet::errorString";
    return QString::fromStdString(m_walletImpl->errorString());
}

bool Wallet::setPassword(const QString &password)
{
qCritical() << "bool Wallet::setPassword";
    return m_walletImpl->setPassword(password.toStdString());
}

QString Wallet::address(quint32 accountIndex, quint32 addressIndex) const
{
qCritical() << "QString Wallet::address";
    return QString::fromStdString(m_walletImpl->address(accountIndex, addressIndex));
}

QString Wallet::path() const
{
qCritical() << "QString Wallet::path";
    return QString::fromStdString(m_walletImpl->path());
}

bool Wallet::store(const QString &path)
{
qCritical() << "bool Wallet::store";
    return m_walletImpl->store(path.toStdString());
}

bool Wallet::init(const QString &daemonAddress, bool trustedDaemon, quint64 upperTransactionLimit, bool isRecovering, bool isRecoveringFromDevice, quint64 restoreHeight)
{
qCritical() << "bool Wallet::init";
    qDebug() << "init non async";
    if (isRecovering){
        qDebug() << "RESTORING";
        m_walletImpl->setRecoveringFromSeed(true);
    }
    if (isRecoveringFromDevice){
        qDebug() << "RESTORING FROM DEVICE";
        m_walletImpl->setRecoveringFromDevice(true);
    }
    if (isRecovering || isRecoveringFromDevice) {
        m_walletImpl->setRefreshFromBlockHeight(restoreHeight);
    }
    m_walletImpl->init(daemonAddress.toStdString(), upperTransactionLimit, m_daemonUsername.toStdString(), m_daemonPassword.toStdString());
    setTrustedDaemon(trustedDaemon);
    return true;
}

void Wallet::setDaemonLogin(const QString &daemonUsername, const QString &daemonPassword)
{
qCritical() << "void Wallet::setDaemonLogin";
    // store daemon login
    m_daemonUsername = daemonUsername;
    m_daemonPassword = daemonPassword;
}

void Wallet::initAsync(const QString &daemonAddress, bool trustedDaemon, quint64 upperTransactionLimit, bool isRecovering, bool isRecoveringFromDevice, quint64 restoreHeight)
{
qCritical() << "void Wallet::initAsync";
    qDebug() << "initAsync: " + daemonAddress;
    // Change status to disconnected if connected
    if(m_connectionStatus != Wallet::ConnectionStatus_Disconnected) {
        m_connectionStatus = Wallet::ConnectionStatus_Disconnected;
        emit connectionStatusChanged(m_connectionStatus);
    }

    m_scheduler.run([this, daemonAddress, trustedDaemon, upperTransactionLimit, isRecovering, isRecoveringFromDevice, restoreHeight] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        bool success = init(daemonAddress, trustedDaemon, upperTransactionLimit, isRecovering, isRecoveringFromDevice, restoreHeight);
        if (success)
        {
            emit walletCreationHeightChanged();
            qDebug() << "init async finished - starting refresh";
            connected(true);
            m_walletImpl->startRefresh();
        }
    });
}

bool Wallet::isHwBacked() const
{
qCritical() << "bool Wallet::isHwBacked";
    return m_walletImpl->getDeviceType() != Monero::Wallet::Device_Software;
}

bool Wallet::isLedger() const
{
qCritical() << "bool Wallet::isLedger";
    return m_walletImpl->getDeviceType() == Monero::Wallet::Device_Ledger;
}

//! create a view only wallet
bool Wallet::createViewOnly(const QString &path, const QString &password) const
{
qCritical() << "bool Wallet::createViewOnly";
    // Create path
    QDir d = QFileInfo(path).absoluteDir();
    d.mkpath(d.absolutePath());
    return m_walletImpl->createWatchOnly(path.toStdString(),password.toStdString(),m_walletImpl->getSeedLanguage());
}

bool Wallet::connectToDaemon()
{
qCritical() << "bool Wallet::connectToDaemon";
    return m_walletImpl->connectToDaemon();
}

void Wallet::setTrustedDaemon(bool arg)
{
qCritical() << "void Wallet::setTrustedDaemon";
    m_walletImpl->setTrustedDaemon(arg);
}

bool Wallet::viewOnly() const
{
qCritical() << "bool Wallet::viewOnly";
    return m_walletImpl->watchOnly();
}

quint64 Wallet::balance(quint32 accountIndex) const
{
qCritical() << "quint64 Wallet::balance";
    return m_walletImpl->balance(accountIndex);
}

quint64 Wallet::balanceAll() const
{
qCritical() << "quint64 Wallet::balanceAll";
    return m_walletImpl->balanceAll();
}

quint64 Wallet::unlockedBalance(quint32 accountIndex) const
{
qCritical() << "quint64 Wallet::unlockedBalance";
    return m_walletImpl->unlockedBalance(accountIndex);
}

quint64 Wallet::unlockedBalanceAll() const
{
qCritical() << "quint64 Wallet::unlockedBalanceAll";
    return m_walletImpl->unlockedBalanceAll();
}

quint32 Wallet::currentSubaddressAccount() const
{
qCritical() << "quint32 Wallet::currentSubaddressAccount";
    return m_currentSubaddressAccount;
}
void Wallet::switchSubaddressAccount(quint32 accountIndex)
{
qCritical() << "void Wallet::switchSubaddressAccount";
    if (accountIndex < numSubaddressAccounts())
    {
        m_currentSubaddressAccount = accountIndex;
        m_subaddress->refresh(m_currentSubaddressAccount);
        m_history->refresh(m_currentSubaddressAccount);
    }
}
void Wallet::addSubaddressAccount(const QString& label)
{
qCritical() << "void Wallet::addSubaddressAccount";
    m_walletImpl->addSubaddressAccount(label.toStdString());
    switchSubaddressAccount(numSubaddressAccounts() - 1);
}
quint32 Wallet::numSubaddressAccounts() const
{
qCritical() << "quint32 Wallet::numSubaddressAccounts";
    return m_walletImpl->numSubaddressAccounts();
}
quint32 Wallet::numSubaddresses(quint32 accountIndex) const
{
qCritical() << "quint32 Wallet::numSubaddresses";
    return m_walletImpl->numSubaddresses(accountIndex);
}
void Wallet::addSubaddress(const QString& label)
{
qCritical() << "void Wallet::addSubaddress";
    m_walletImpl->addSubaddress(currentSubaddressAccount(), label.toStdString());
}
QString Wallet::getSubaddressLabel(quint32 accountIndex, quint32 addressIndex) const
{
qCritical() << "QString Wallet::getSubaddressLabel";
    return QString::fromStdString(m_walletImpl->getSubaddressLabel(accountIndex, addressIndex));
}
void Wallet::setSubaddressLabel(quint32 accountIndex, quint32 addressIndex, const QString &label)
{
qCritical() << "void Wallet::setSubaddressLabel";
    m_walletImpl->setSubaddressLabel(accountIndex, addressIndex, label.toStdString());
}
void Wallet::deviceShowAddressAsync(quint32 accountIndex, quint32 addressIndex, const QString &paymentId)
{
qCritical() << "void Wallet::deviceShowAddressAsync";
    m_scheduler.run([this, accountIndex, addressIndex, paymentId] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        m_walletImpl->deviceShowAddress(accountIndex, addressIndex, paymentId.toStdString());
        emit deviceShowAddressShowed();
    });
}

void Wallet::refreshHeightAsync()
{
qCritical() << "void Wallet::refreshHeightAsync";
    m_scheduler.run([this] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        quint64 daemonHeight;
        QPair<bool, QFuture<void>> daemonHeightFuture = m_scheduler.run([this, &daemonHeight] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
            daemonHeight = daemonBlockChainHeight();
        });
        if (!daemonHeightFuture.first)
        {
            return;
        }

        quint64 targetHeight;
        QPair<bool, QFuture<void>> targetHeightFuture = m_scheduler.run([this, &targetHeight] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
            targetHeight = daemonBlockChainTargetHeight();
        });
        if (!targetHeightFuture.first)
        {
            return;
        }

        quint64 walletHeight = blockChainHeight();
        daemonHeightFuture.second.waitForFinished();
        targetHeightFuture.second.waitForFinished();

        emit heightRefreshed(walletHeight, daemonHeight, targetHeight);
    });
}

quint64 Wallet::blockChainHeight() const
{
qCritical() << "quint64 Wallet::blockChainHeight";
    return m_walletImpl->blockChainHeight();
}

quint64 Wallet::daemonBlockChainHeight() const
{
qCritical() << "quint64 Wallet::daemonBlockChainHeight";
    // cache daemon blockchain height for some time (60 seconds by default)

    if (m_daemonBlockChainHeight == 0
            || m_daemonBlockChainHeightTime.elapsed() / 1000 > m_daemonBlockChainHeightTtl) {
        m_daemonBlockChainHeight = m_walletImpl->daemonBlockChainHeight();
        m_daemonBlockChainHeightTime.restart();
    }
    return m_daemonBlockChainHeight;
}

quint64 Wallet::daemonBlockChainTargetHeight() const
{
qCritical() << "quint64 Wallet::daemonBlockChainTargetHeight";
    if (m_daemonBlockChainTargetHeight <= 1
            || m_daemonBlockChainTargetHeightTime.elapsed() / 1000 > m_daemonBlockChainTargetHeightTtl) {
        m_daemonBlockChainTargetHeight = m_walletImpl->daemonBlockChainTargetHeight();

        // Target height is set to 0 if daemon is synced.
        // Use current height from daemon when target height < current height
        if (m_daemonBlockChainTargetHeight < m_daemonBlockChainHeight){
            m_daemonBlockChainTargetHeight = m_daemonBlockChainHeight;
        }
        m_daemonBlockChainTargetHeightTime.restart();
    }

    return m_daemonBlockChainTargetHeight;
}

bool Wallet::exportKeyImages(const QString& path)
{
qCritical() << "bool Wallet::exportKeyImages";
    return m_walletImpl->exportKeyImages(path.toStdString());
}

bool Wallet::importKeyImages(const QString& path)
{
qCritical() << "bool Wallet::importKeyImages";
    return m_walletImpl->importKeyImages(path.toStdString());
}

bool Wallet::refresh()
{
qCritical() << "bool Wallet::refresh";
    bool result = m_walletImpl->refresh();
    m_history->refresh(currentSubaddressAccount());
    m_subaddress->refresh(currentSubaddressAccount());
    m_subaddressAccount->getAll(true);
    if (result)
        emit updated();
    return result;
}

void Wallet::refreshAsync()
{
qCritical() << "void Wallet::refreshAsync";
    qDebug() << "refresh async";
    m_walletImpl->refreshAsync();
}

void Wallet::setAutoRefreshInterval(int seconds)
{
qCritical() << "void Wallet::setAutoRefreshInterval";
    m_walletImpl->setAutoRefreshInterval(seconds);
}

int Wallet::autoRefreshInterval() const
{
qCritical() << "int Wallet::autoRefreshInterval";
    return m_walletImpl->autoRefreshInterval();
}

void Wallet::startRefresh() const
{
qCritical() << "void Wallet::startRefresh";
    m_walletImpl->startRefresh();
}

void Wallet::pauseRefresh() const
{
qCritical() << "void Wallet::pauseRefresh";
    m_walletImpl->pauseRefresh();
}

PendingTransaction *Wallet::createTransaction(const QString &dst_addr, const QString &payment_id,
                                              quint64 amount, quint32 mixin_count,
                                              PendingTransaction::Priority priority)
{
    std::set<uint32_t> subaddr_indices;
    Monero::PendingTransaction * ptImpl = m_walletImpl->createTransaction(
                dst_addr.toStdString(), payment_id.toStdString(), amount, mixin_count,
                static_cast<Monero::PendingTransaction::Priority>(priority), currentSubaddressAccount(), subaddr_indices);
    PendingTransaction * result = new PendingTransaction(ptImpl,0);
    return result;
}

void Wallet::createTransactionAsync(const QString &dst_addr, const QString &payment_id,
                               quint64 amount, quint32 mixin_count,
                               PendingTransaction::Priority priority)
{
    m_scheduler.run([this, dst_addr, payment_id, amount, mixin_count, priority] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        PendingTransaction *tx = createTransaction(dst_addr, payment_id, amount, mixin_count, priority);
        emit transactionCreated(tx, dst_addr, payment_id, mixin_count);
    });
}

PendingTransaction *Wallet::createTransactionAll(const QString &dst_addr, const QString &payment_id,
                                                 quint32 mixin_count, PendingTransaction::Priority priority)
{
    std::set<uint32_t> subaddr_indices;
    Monero::PendingTransaction * ptImpl = m_walletImpl->createTransaction(
                dst_addr.toStdString(), payment_id.toStdString(), Monero::optional<uint64_t>(), mixin_count,
                static_cast<Monero::PendingTransaction::Priority>(priority), currentSubaddressAccount(), subaddr_indices);
    PendingTransaction * result = new PendingTransaction(ptImpl, this);
    return result;
}

void Wallet::createTransactionAllAsync(const QString &dst_addr, const QString &payment_id,
                               quint32 mixin_count,
                               PendingTransaction::Priority priority)
{
    m_scheduler.run([this, dst_addr, payment_id, mixin_count, priority] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        PendingTransaction *tx = createTransactionAll(dst_addr, payment_id, mixin_count, priority);
        emit transactionCreated(tx, dst_addr, payment_id, mixin_count);
    });
}

PendingTransaction *Wallet::createSweepUnmixableTransaction()
{
    Monero::PendingTransaction * ptImpl = m_walletImpl->createSweepUnmixableTransaction();
    PendingTransaction * result = new PendingTransaction(ptImpl, this);
    return result;
}

void Wallet::createSweepUnmixableTransactionAsync()
{
qCritical() << "void Wallet::createSweepUnmixableTransactionAsync";
    m_scheduler.run([this] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        PendingTransaction *tx = createSweepUnmixableTransaction();
        emit transactionCreated(tx, "", "", 0);
    });
}

UnsignedTransaction * Wallet::loadTxFile(const QString &fileName)
{
    qDebug() << "Trying to sign " << fileName;
    Monero::UnsignedTransaction * ptImpl = m_walletImpl->loadUnsignedTx(fileName.toStdString());
    UnsignedTransaction * result = new UnsignedTransaction(ptImpl, m_walletImpl, this);
    return result;
}

bool Wallet::submitTxFile(const QString &fileName) const
{
qCritical() << "bool Wallet::submitTxFile";
    qDebug() << "Trying to submit " << fileName;
    if (!m_walletImpl->submitTransaction(fileName.toStdString()))
        return false;
    // import key images
    return m_walletImpl->importKeyImages(fileName.toStdString() + "_keyImages");
}

void Wallet::commitTransactionAsync(PendingTransaction *t)
{
qCritical() << "void Wallet::commitTransactionAsync";
    m_scheduler.run([this, t] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        auto txIdList = t->txid();  // retrieve before commit
        emit transactionCommitted(t->commit(), t, txIdList);
    });
}

void Wallet::disposeTransaction(PendingTransaction *t)
{
qCritical() << "void Wallet::disposeTransaction";
    m_walletImpl->disposeTransaction(t->m_pimpl);
    delete t;
}

void Wallet::disposeTransaction(UnsignedTransaction *t)
{
qCritical() << "void Wallet::disposeTransaction";
    delete t;
}

TransactionHistory *Wallet::history() const
{
    return m_history;
}

TransactionHistorySortFilterModel *Wallet::historyModel() const
{
    if (!m_historyModel) {
        Wallet * w = const_cast<Wallet*>(this);
        m_historyModel = new TransactionHistoryModel(w);
        m_historyModel->setTransactionHistory(this->history());
        m_historySortFilterModel = new TransactionHistorySortFilterModel(w);
        m_historySortFilterModel->setSourceModel(m_historyModel);
        m_historySortFilterModel->setSortRole(TransactionHistoryModel::TransactionBlockHeightRole);
        m_historySortFilterModel->sort(0, Qt::DescendingOrder);
    }

    return m_historySortFilterModel;
}

AddressBook *Wallet::addressBook() const
{
    return m_addressBook;
}

AddressBookModel *Wallet::addressBookModel() const
{

    if (!m_addressBookModel) {
        Wallet * w = const_cast<Wallet*>(this);
        m_addressBookModel = new AddressBookModel(w,m_addressBook);
    }

    return m_addressBookModel;
}

Subaddress *Wallet::subaddress()
{
    return m_subaddress;
}

SubaddressModel *Wallet::subaddressModel()
{
    if (!m_subaddressModel) {
        m_subaddressModel = new SubaddressModel(this, m_subaddress);
    }
    return m_subaddressModel;
}

SubaddressAccount *Wallet::subaddressAccount() const
{
    return m_subaddressAccount;
}

SubaddressAccountModel *Wallet::subaddressAccountModel() const
{
    if (!m_subaddressAccountModel) {
        Wallet * w = const_cast<Wallet*>(this);
        m_subaddressAccountModel = new SubaddressAccountModel(w,m_subaddressAccount);
    }
    return m_subaddressAccountModel;
}

QString Wallet::generatePaymentId() const
{
qCritical() << "QString Wallet::generatePaymentId";
    return QString::fromStdString(Monero::Wallet::genPaymentId());
}

QString Wallet::integratedAddress(const QString &paymentId) const
{
qCritical() << "QString Wallet::integratedAddress";
    return QString::fromStdString(m_walletImpl->integratedAddress(paymentId.toStdString()));
}

QString Wallet::paymentId() const
{
qCritical() << "QString Wallet::paymentId";
    return m_paymentId;
}

void Wallet::setPaymentId(const QString &paymentId)
{
qCritical() << "void Wallet::setPaymentId";
    m_paymentId = paymentId;
}

QString Wallet::getCacheAttribute(const QString &key) const {
    return QString::fromStdString(m_walletImpl->getCacheAttribute(key.toStdString()));
}

bool Wallet::setCacheAttribute(const QString &key, const QString &val)
{
qCritical() << "bool Wallet::setCacheAttribute";
    return m_walletImpl->setCacheAttribute(key.toStdString(), val.toStdString());
}

bool Wallet::setUserNote(const QString &txid, const QString &note)
{
qCritical() << "bool Wallet::setUserNote";
  return m_walletImpl->setUserNote(txid.toStdString(), note.toStdString());
}

QString Wallet::getUserNote(const QString &txid) const
{
qCritical() << "QString Wallet::getUserNote";
  return QString::fromStdString(m_walletImpl->getUserNote(txid.toStdString()));
}

QString Wallet::getTxKey(const QString &txid) const
{
qCritical() << "QString Wallet::getTxKey";
  return QString::fromStdString(m_walletImpl->getTxKey(txid.toStdString()));
}

void Wallet::getTxKeyAsync(const QString &txid, const QJSValue &callback)
{
qCritical() << "void Wallet::getTxKeyAsync";
    m_scheduler.run([this, txid] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        return QJSValueList({txid, getTxKey(txid)});
    }, callback);
}

QString Wallet::checkTxKey(const QString &txid, const QString &tx_key, const QString &address)
{
qCritical() << "QString Wallet::checkTxKey";
    uint64_t received;
    bool in_pool;
    uint64_t confirmations;
    bool success = m_walletImpl->checkTxKey(txid.toStdString(), tx_key.toStdString(), address.toStdString(), received, in_pool, confirmations);
    std::string result = std::string(success ? "true" : "false") + "|" + QString::number(received).toStdString() + "|" + std::string(in_pool ? "true" : "false") + "|" + QString::number(confirmations).toStdString();
    return QString::fromStdString(result);
}

QString Wallet::getTxProof(const QString &txid, const QString &address, const QString &message) const
{
qCritical() << "QString Wallet::getTxProof";
    std::string result = m_walletImpl->getTxProof(txid.toStdString(), address.toStdString(), message.toStdString());
    if (result.empty())
        result = "error|" + m_walletImpl->errorString();
    return QString::fromStdString(result);
}

void Wallet::getTxProofAsync(const QString &txid, const QString &address, const QString &message, const QJSValue &callback)
{
qCritical() << "void Wallet::getTxProofAsync";
    m_scheduler.run([this, txid, address, message] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        return QJSValueList({txid, getTxProof(txid, address, message)});
    }, callback);
}

QString Wallet::checkTxProof(const QString &txid, const QString &address, const QString &message, const QString &signature)
{
qCritical() << "QString Wallet::checkTxProof";
    bool good;
    uint64_t received;
    bool in_pool;
    uint64_t confirmations;
    bool success = m_walletImpl->checkTxProof(txid.toStdString(), address.toStdString(), message.toStdString(), signature.toStdString(), good, received, in_pool, confirmations);
    std::string result = std::string(success ? "true" : "false") + "|" + std::string(good ? "true" : "false") + "|" + QString::number(received).toStdString() + "|" + std::string(in_pool ? "true" : "false") + "|" + QString::number(confirmations).toStdString();
    return QString::fromStdString(result);
}

Q_INVOKABLE QString Wallet::getSpendProof(const QString &txid, const QString &message) const
{
qCritical() << "QString Wallet::getSpendProof";
    std::string result = m_walletImpl->getSpendProof(txid.toStdString(), message.toStdString());
    if (result.empty())
        result = "error|" + m_walletImpl->errorString();
    return QString::fromStdString(result);
}

void Wallet::getSpendProofAsync(const QString &txid, const QString &message, const QJSValue &callback)
{
qCritical() << "void Wallet::getSpendProofAsync";
    m_scheduler.run([this, txid, message] {
qCritical() << "async " << __FILE__ << ":" << __LINE__;
        return QJSValueList({txid, getSpendProof(txid, message)});
    }, callback);
}

Q_INVOKABLE QString Wallet::checkSpendProof(const QString &txid, const QString &message, const QString &signature) const
{
qCritical() << "QString Wallet::checkSpendProof";
    bool good;
    bool success = m_walletImpl->checkSpendProof(txid.toStdString(), message.toStdString(), signature.toStdString(), good);
    std::string result = std::string(success ? "true" : "false") + "|" + std::string(!success ? m_walletImpl->errorString() : good ? "true" : "false");
    return QString::fromStdString(result);
}

QString Wallet::signMessage(const QString &message, bool filename) const
{
qCritical() << "QString Wallet::signMessage";
  if (filename) {
    QFile file(message);
    uchar *data = NULL;

    try {
      if (!file.open(QIODevice::ReadOnly))
        return "";
      quint64 size = file.size();
      if (size == 0) {
        file.close();
        return QString::fromStdString(m_walletImpl->signMessage(std::string()));
      }
      data = file.map(0, size);
      if (!data) {
        file.close();
        return "";
      }
      std::string signature = m_walletImpl->signMessage(std::string((const char*)data, size));
      file.unmap(data);
      file.close();
      return QString::fromStdString(signature);
    }
    catch (const std::exception &e) {
      if (data)
        file.unmap(data);
      file.close();
      return "";
    }
  }
  else {
    return QString::fromStdString(m_walletImpl->signMessage(message.toStdString()));
  }
}

bool Wallet::verifySignedMessage(const QString &message, const QString &address, const QString &signature, bool filename) const
{
qCritical() << "bool Wallet::verifySignedMessage";
  if (filename) {
    QFile file(message);
    uchar *data = NULL;

    try {
      if (!file.open(QIODevice::ReadOnly))
        return false;
      quint64 size = file.size();
      if (size == 0) {
        file.close();
        return m_walletImpl->verifySignedMessage(std::string(), address.toStdString(), signature.toStdString());
      }
      data = file.map(0, size);
      if (!data) {
        file.close();
        return false;
      }
      bool ret = m_walletImpl->verifySignedMessage(std::string((const char*)data, size), address.toStdString(), signature.toStdString());
      file.unmap(data);
      file.close();
      return ret;
    }
    catch (const std::exception &e) {
      if (data)
        file.unmap(data);
      file.close();
      return false;
    }
  }
  else {
    return m_walletImpl->verifySignedMessage(message.toStdString(), address.toStdString(), signature.toStdString());
  }
}
bool Wallet::parse_uri(const QString &uri, QString &address, QString &payment_id, uint64_t &amount, QString &tx_description, QString &recipient_name, QVector<QString> &unknown_parameters, QString &error)
{
qCritical() << "bool Wallet::parse_uri";
   std::string s_address, s_payment_id, s_tx_description, s_recipient_name, s_error;
   std::vector<std::string> s_unknown_parameters;
   bool res= m_walletImpl->parse_uri(uri.toStdString(), s_address, s_payment_id, amount, s_tx_description, s_recipient_name, s_unknown_parameters, s_error);
   if(res)
   {
       address = QString::fromStdString(s_address);
       payment_id = QString::fromStdString(s_payment_id);
       tx_description = QString::fromStdString(s_tx_description);
       recipient_name = QString::fromStdString(s_recipient_name);
       for( const auto &p : s_unknown_parameters )
           unknown_parameters.append(QString::fromStdString(p));
   }
   error = QString::fromStdString(s_error);
   return res;
}

bool Wallet::rescanSpent()
{
qCritical() << "bool Wallet::rescanSpent";
    return m_walletImpl->rescanSpent();
}

bool Wallet::useForkRules(quint8 required_version, quint64 earlyBlocks) const
{
qCritical() << "bool Wallet::useForkRules";
    if(m_connectionStatus == Wallet::ConnectionStatus_Disconnected)
        return false;
    try {
        return m_walletImpl->useForkRules(required_version,earlyBlocks);
    } catch (const std::exception &e) {
        qDebug() << e.what();
        return false;
    }
}

void Wallet::setWalletCreationHeight(quint64 height)
{
qCritical() << "void Wallet::setWalletCreationHeight";
    m_walletImpl->setRefreshFromBlockHeight(height);
    emit walletCreationHeightChanged();
}

QString Wallet::getDaemonLogPath() const
{
qCritical() << "QString Wallet::getDaemonLogPath";
    return QString::fromStdString(m_walletImpl->getDefaultDataDir()) + "/bitmonero.log";
}

bool Wallet::blackballOutput(const QString &amount, const QString &offset)
{
qCritical() << "bool Wallet::blackballOutput";
    return m_walletImpl->blackballOutput(amount.toStdString(), offset.toStdString());
}

bool Wallet::blackballOutputs(const QList<QString> &pubkeys, bool add)
{
qCritical() << "bool Wallet::blackballOutputs";
    std::vector<std::string> std_pubkeys;
    foreach (const QString &pubkey, pubkeys) {
        std_pubkeys.push_back(pubkey.toStdString());
    }
    return m_walletImpl->blackballOutputs(std_pubkeys, add);
}

bool Wallet::blackballOutputs(const QString &filename, bool add)
{
qCritical() << "bool Wallet::blackballOutputs";
    QFile file(filename);

    try {
        if (!file.open(QIODevice::ReadOnly))
            return false;
        QList<QString> outputs;
        QTextStream in(&file);
        while (!in.atEnd()) {
            outputs.push_back(in.readLine());
        }
        file.close();
        return blackballOutputs(outputs, add);
    }
    catch (const std::exception &e) {
        file.close();
        return false;
    }
}

bool Wallet::unblackballOutput(const QString &amount, const QString &offset)
{
qCritical() << "bool Wallet::unblackballOutput";
    return m_walletImpl->unblackballOutput(amount.toStdString(), offset.toStdString());
}

QString Wallet::getRing(const QString &key_image)
{
qCritical() << "QString Wallet::getRing";
    std::vector<uint64_t> cring;
    if (!m_walletImpl->getRing(key_image.toStdString(), cring))
        return "";
    QString ring = "";
    for (uint64_t out: cring)
    {
        if (!ring.isEmpty())
            ring = ring + " ";
	QString s;
	s.setNum(out);
        ring = ring + s;
    }
    return ring;
}

QString Wallet::getRings(const QString &txid)
{
qCritical() << "QString Wallet::getRings";
    std::vector<std::pair<std::string, std::vector<uint64_t>>> crings;
    if (!m_walletImpl->getRings(txid.toStdString(), crings))
        return "";
    QString ring = "";
    for (const auto &cring: crings)
    {
        if (!ring.isEmpty())
            ring = ring + "|";
        ring = ring + QString::fromStdString(cring.first) + " absolute";
        for (uint64_t out: cring.second)
        {
            ring = ring + " ";
	    QString s;
	    s.setNum(out);
            ring = ring + s;
        }
    }
    return ring;
}

bool Wallet::setRing(const QString &key_image, const QString &ring, bool relative)
{
qCritical() << "bool Wallet::setRing";
    std::vector<uint64_t> cring;
    QStringList strOuts = ring.split(" ");
    foreach(QString str, strOuts)
    {
        uint64_t out;
	bool ok;
	out = str.toULong(&ok);
	if (ok)
            cring.push_back(out);
    }
    return m_walletImpl->setRing(key_image.toStdString(), cring, relative);
}

void Wallet::segregatePreForkOutputs(bool segregate)
{
qCritical() << "void Wallet::segregatePreForkOutputs";
    m_walletImpl->segregatePreForkOutputs(segregate);
}

void Wallet::segregationHeight(quint64 height)
{
qCritical() << "void Wallet::segregationHeight";
    m_walletImpl->segregationHeight(height);
}

void Wallet::keyReuseMitigation2(bool mitigation)
{
qCritical() << "void Wallet::keyReuseMitigation2";
    m_walletImpl->keyReuseMitigation2(mitigation);
}

Wallet::Wallet(Monero::Wallet *w, QObject *parent)
    : QObject(parent)
    , m_walletImpl(w)
    , m_history(nullptr)
    , m_historyModel(nullptr)
    , m_addressBook(nullptr)
    , m_addressBookModel(nullptr)
    , m_daemonBlockChainHeight(0)
    , m_daemonBlockChainHeightTtl(DAEMON_BLOCKCHAIN_HEIGHT_CACHE_TTL_SECONDS)
    , m_daemonBlockChainTargetHeight(0)
    , m_daemonBlockChainTargetHeightTtl(DAEMON_BLOCKCHAIN_TARGET_HEIGHT_CACHE_TTL_SECONDS)
    , m_connectionStatusTtl(WALLET_CONNECTION_STATUS_CACHE_TTL_SECONDS)
    , m_currentSubaddressAccount(0)
    , m_subaddress(nullptr)
    , m_subaddressModel(nullptr)
    , m_subaddressAccount(nullptr)
    , m_subaddressAccountModel(nullptr)
    , m_scheduler(this)
{
    m_history = new TransactionHistory(m_walletImpl->history(), this);
    m_addressBook = new AddressBook(m_walletImpl->addressBook(), this);
    m_subaddress = new Subaddress(m_walletImpl->subaddress(), this);
    m_subaddressAccount = new SubaddressAccount(m_walletImpl->subaddressAccount(), this);
    m_walletListener = new WalletListenerImpl(this);
    m_walletImpl->setListener(m_walletListener);
    m_connectionStatus = Wallet::ConnectionStatus_Disconnected;
    // start cache timers
    m_connectionStatusTime.restart();
    m_daemonBlockChainHeightTime.restart();
    m_daemonBlockChainTargetHeightTime.restart();
    m_initialized = false;
    m_connectionStatusRunning = false;
    m_daemonUsername = "";
    m_daemonPassword = "";
}

Wallet::~Wallet()
{
    qDebug("~Wallet: Closing wallet");

    m_scheduler.shutdownWaitForFinished();

    delete m_addressBook;
    m_addressBook = NULL;

    delete m_history;
    m_history = NULL;
    delete m_addressBook;
    m_addressBook = NULL;
    delete m_subaddress;
    m_subaddress = NULL;
    delete m_subaddressAccount;
    m_subaddressAccount = NULL;
    //Monero::WalletManagerFactory::getWalletManager()->closeWallet(m_walletImpl);
    if(status() == Status_Critical)
        qDebug("Not storing wallet cache");
    else if( m_walletImpl->store(""))
        qDebug("Wallet cache stored successfully");
    else
        qDebug("Error storing wallet cache");
    delete m_walletImpl;
    m_walletImpl = NULL;
    delete m_walletListener;
    m_walletListener = NULL;
    qDebug("m_walletImpl deleted");
}

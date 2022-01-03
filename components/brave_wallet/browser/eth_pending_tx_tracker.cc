/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

EthPendingTxTracker::EthPendingTxTracker(EthTxStateManager* tx_state_manager,
                                         EthJsonRpcController* rpc_controller,
                                         EthNonceTracker* nonce_tracker)
    : tx_state_manager_(tx_state_manager),
      rpc_controller_(rpc_controller),
      nonce_tracker_(nonce_tracker),
      weak_factory_(this) {}
EthPendingTxTracker::~EthPendingTxTracker() = default;

void EthPendingTxTracker::UpdatePendingTransactions(
    UpdatePendingTransactionsCallback callback) {
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try()) {
    std::move(callback).Run(false, 0);
    return;
  }

  tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Confirmed, absl::nullopt,
      base::BindOnce(&EthPendingTxTracker::ContinueUpdatePendingTransactions,
                     weak_factory_.GetWeakPtr(), std::move(callback)));

  nonce_lock->Release();
}

void EthPendingTxTracker::ContinueUpdatePendingTransactions(
    UpdatePendingTransactionsCallback callback,
    std::vector<std::unique_ptr<EthTxStateManager::TxMeta>> confirmed_txs) {
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try()) {
    std::move(callback).Run(false, 0);
    return;
  }
  tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt,
      base::BindOnce(&EthPendingTxTracker::FinalizeUpdatePendingTransactions,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(confirmed_txs)));
  nonce_lock->Release();
}

void EthPendingTxTracker::FinalizeUpdatePendingTransactions(
    UpdatePendingTransactionsCallback callback,
    std::vector<std::unique_ptr<EthTxStateManager::TxMeta>> confirmed_txs,
    std::vector<std::unique_ptr<EthTxStateManager::TxMeta>> pending_txs) {
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try()) {
    std::move(callback).Run(false, 0);
    return;
  }

  for (const auto& pending_tx : pending_txs) {
    bool is_nonce_taken = false;
    for (const auto& confirmed_tx : confirmed_txs) {
      if (confirmed_tx->tx->nonce() == pending_tx->tx->nonce() &&
          confirmed_tx->id != pending_tx->id) {
        DropTransaction(pending_tx.get());
        is_nonce_taken = true;
      }
    }
    if (is_nonce_taken)
      continue;
    std::string id = pending_tx->id;
    rpc_controller_->GetTransactionReceipt(
        pending_tx->tx_hash,
        base::BindOnce(&EthPendingTxTracker::OnGetTxReceipt,
                       weak_factory_.GetWeakPtr(), std::move(id)));
  }
  nonce_lock->Release();
  std::move(callback).Run(true, pending_txs.size());
}

void EthPendingTxTracker::ResubmitPendingTransactions() {
  // TODO(darkdh): limit the rate of tx publishing
  tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt,
      base::BindOnce(&EthPendingTxTracker::ContinueResubmitPendingTransactions,
                     weak_factory_.GetWeakPtr()));
}

void EthPendingTxTracker::ContinueResubmitPendingTransactions(
    std::vector<std::unique_ptr<EthTxStateManager::TxMeta>> pending_txs) {
  for (const auto& pending_tx : pending_txs) {
    if (!pending_tx->tx->IsSigned()) {
      continue;
    }
    rpc_controller_->SendRawTransaction(
        pending_tx->tx->GetSignedTransaction(),
        base::BindOnce(&EthPendingTxTracker::OnSendRawTransaction,
                       weak_factory_.GetWeakPtr()));
  }
}

void EthPendingTxTracker::Reset() {
  network_nonce_map_.clear();
  dropped_blocks_counter_.clear();
}

void EthPendingTxTracker::OnGetTxReceipt(std::string id,
                                         TransactionReceipt receipt,
                                         mojom::ProviderError error,
                                         const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess)
    return;

  tx_state_manager_->GetTx(
      id,
      base::BindOnce(&EthPendingTxTracker::ContinueOnGetTxReceipt,
                     weak_factory_.GetWeakPtr(), error, std::move(receipt)));
}

void EthPendingTxTracker::ContinueOnGetTxReceipt(
    mojom::ProviderError error,
    TransactionReceipt receipt,
    std::unique_ptr<EthTxStateManager::TxMeta> meta) {
  base::Lock* nonce_lock = nonce_tracker_->GetLock();

  if (!meta || !nonce_lock->Try()) {
    return;
  }
  if (receipt.status) {
    meta->tx_receipt = receipt;
    meta->status = mojom::TransactionStatus::Confirmed;
    meta->confirmed_time = base::Time::Now();
    tx_state_manager_->AddOrUpdateTx(std::move(meta));
  } else if (ShouldTxDropped(*meta)) {
    DropTransaction(meta.get());
  }

  nonce_lock->Release();
}  // namespace brave_wallet

void EthPendingTxTracker::OnGetNetworkNonce(std::string address,
                                            uint256_t result,
                                            mojom::ProviderError error,
                                            const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess)
    return;

  network_nonce_map_[address] = result;
}

void EthPendingTxTracker::OnSendRawTransaction(
    const std::string& tx_hash,
    mojom::ProviderError error,
    const std::string& error_message) {}

bool EthPendingTxTracker::ShouldTxDropped(
    const EthTxStateManager::TxMeta& meta) {
  const std::string hex_address = meta.from.ToChecksumAddress();
  if (network_nonce_map_.find(hex_address) == network_nonce_map_.end()) {
    rpc_controller_->GetTransactionCount(
        hex_address,
        base::BindOnce(&EthPendingTxTracker::OnGetNetworkNonce,
                       weak_factory_.GetWeakPtr(), std::move(hex_address)));
  } else {
    uint256_t network_nonce = network_nonce_map_[hex_address];
    network_nonce_map_.erase(hex_address);
    if (meta.tx->nonce() < network_nonce)
      return true;
  }

  const std::string tx_hash = meta.tx_hash;
  if (dropped_blocks_counter_.find(tx_hash) == dropped_blocks_counter_.end()) {
    dropped_blocks_counter_[tx_hash] = 0;
  }
  if (dropped_blocks_counter_[tx_hash] >= 3) {
    dropped_blocks_counter_.erase(tx_hash);
    return true;
  }

  dropped_blocks_counter_[tx_hash] = dropped_blocks_counter_[tx_hash] + 1;

  return false;
}

void EthPendingTxTracker::DropTransaction(EthTxStateManager::TxMeta* meta) {
  if (!meta)
    return;
  tx_state_manager_->DeleteTx(meta->id);
}

}  // namespace brave_wallet

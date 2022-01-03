/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/observer_list.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace value_store {
class ValueStoreFactory;
class ValueStoreFrontend;
}  // namespace value_store

namespace brave_wallet {

class EthJsonRpcController;

class EthTxStateManager : public mojom::EthJsonRpcControllerObserver {
 public:
  struct TxMeta {
    TxMeta();
    explicit TxMeta(std::unique_ptr<EthTransaction> tx);
    TxMeta(const TxMeta&) = delete;
    TxMeta& operator=(const TxMeta&) = delete;
    ~TxMeta();
    bool operator==(const TxMeta&) const;

    std::string id;
    mojom::TransactionStatus status = mojom::TransactionStatus::Unapproved;
    EthAddress from;
    base::Time created_time;
    base::Time submitted_time;
    base::Time confirmed_time;
    TransactionReceipt tx_receipt;
    std::string tx_hash;
    std::unique_ptr<EthTransaction> tx;
  };
  using GetTxCallback = base::OnceCallback<void(std::unique_ptr<TxMeta>)>;
  using GetTxsByStatusCallback = base::OnceCallback<void(
      std::vector<std::unique_ptr<EthTxStateManager::TxMeta>>)>;

  EthTxStateManager(PrefService* prefs,
                    const base::FilePath context_path,
                    EthJsonRpcController* rpc_controller);
  ~EthTxStateManager() override;
  EthTxStateManager(const EthTxStateManager&) = delete;
  EthTxStateManager operator=(const EthTxStateManager&) = delete;

  static std::string GenerateMetaID();
  static base::Value TxMetaToValue(const TxMeta& meta);
  static mojom::TransactionInfoPtr TxMetaToTransactionInfo(const TxMeta& meta);
  static std::unique_ptr<TxMeta> ValueToTxMeta(const base::Value& value);

  void AddOrUpdateTx(std::unique_ptr<TxMeta> meta);
  void GetTx(const std::string& id, GetTxCallback);
  void DeleteTx(const std::string& id);
  void WipeTxs();

  void GetTransactionsByStatus(absl::optional<mojom::TransactionStatus> status,
                               absl::optional<EthAddress> from,
                               GetTxsByStatusCallback);

  // mojom::EthJsonRpcControllerObserver
  void ChainChangedEvent(const std::string& chain_id) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;
  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
    }
    virtual void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {}
  };
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
#if 0
  // only support REJECTED and CONFIRMED
  void RetireTxByStatus(mojom::TransactionStatus status, size_t max_num);
#endif

  void ContinueAddOrUpdateTx(std::unique_ptr<TxMeta>,
                             std::unique_ptr<base::Value>);
  void ContinueGetTx(const std::string& id,
                     GetTxCallback,
                     std::unique_ptr<base::Value>);
  void ContinueDeleteTx(const std::string& id, std::unique_ptr<base::Value>);
  void ContinueGetTransactionsByStatus(
      absl::optional<mojom::TransactionStatus> status,
      absl::optional<EthAddress> from,
      GetTxsByStatusCallback callback,
      std::unique_ptr<base::Value>);

  base::ObserverList<Observer> observers_;
  PrefService* prefs_;
  EthJsonRpcController* rpc_controller_;
  mojo::Receiver<mojom::EthJsonRpcControllerObserver> observer_receiver_{this};
  std::string chain_id_;
  std::string network_url_;
  scoped_refptr<base::SequencedTaskRunner> store_task_runner_;
  scoped_refptr<value_store::ValueStoreFactory> store_factory_;
  std::unique_ptr<value_store::ValueStoreFrontend> store_;
  base::WeakPtrFactory<EthTxStateManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_

/**
 * Copyright (c) 2017-present, Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */
#pragma once
#include <folly/ThreadLocal.h>

#include "logdevice/common/ClusterState.h"
#include "logdevice/common/HashBasedSequencerLocator.h"
#include "logdevice/common/SequencerRouter.h"
#include "logdevice/common/StaticSequencerLocator.h"
#include "logdevice/common/configuration/Configuration.h"
#include "logdevice/common/configuration/UpdateableConfig.h"
#include "logdevice/common/settings/Settings.h"
#include "logdevice/common/test/TestUtil.h"

namespace facebook { namespace logdevice {

class MockClusterState : public ClusterState {
 public:
  explicit MockClusterState(size_t max_nodes)
      : ClusterState(max_nodes,
                     /*processor=*/nullptr,
                     /*nconfig=*/createConfigFor(max_nodes)) {}

  // override this method to do nothing
  void refreshClusterStateAsync() override {}

 protected:
  static configuration::nodes::ServiceDiscoveryConfig
  createConfigFor(size_t nnodes) {
    using configuration::nodes::NodeServiceDiscovery;
    using configuration::nodes::ServiceDiscoveryConfig;
    constexpr NodeServiceDiscovery::RoleSet kBothRoles{3};
    ServiceDiscoveryConfig sdconfig;

    ServiceDiscoveryConfig::Update update;
    for (node_index_t nid = 0; nid < nnodes; nid++) {
      using NodeUpdate = ServiceDiscoveryConfig::NodeUpdate;
      auto url = folly::sformat("127.0.0.{}", nid);
      auto nsd = std::make_unique<NodeServiceDiscovery>(
          NodeServiceDiscovery{folly::sformat("server-{}", nid),
                               /*version=*/0,
                               Sockaddr(url, 4440),
                               Sockaddr(url, 4441),
                               /*ssl_address=*/folly::none,
                               /*admin_address=*/Sockaddr(url, 6440),
                               /*internal_address=*/folly::none,
                               /*server_thrift_api=*/folly::none,
                               /*client_thrift_api=*/folly::none,
                               /*location=*/folly::none,
                               kBothRoles});
      NodeUpdate nup{
          ServiceDiscoveryConfig::UpdateType::PROVISION, std::move(nsd)};
      update.addNode(nid, std::move(nup));
    }
    ServiceDiscoveryConfig sdconfig2;
    sdconfig.applyUpdate(update, &sdconfig2);
    return sdconfig2;
  }
};

class MockSequencerRouter : public SequencerRouter {
 public:
  MockSequencerRouter(logid_t log_id,
                      Handler* handler,
                      std::shared_ptr<const NodesConfiguration> nodes_config,
                      std::shared_ptr<SequencerLocator> locator,
                      ClusterState* cluster_state)
      : SequencerRouter(log_id, handler),
        settings_(create_default_settings<Settings>()),
        nodes_config_(nodes_config),
        locator_(locator),
        cluster_state_(cluster_state) {
    ld_check(locator_ != nullptr);
  }

  ~MockSequencerRouter() override {}

  std::shared_ptr<const configuration::nodes::NodesConfiguration>
  getNodesConfiguration() const override {
    return nodes_config_;
  }

  const Settings& getSettings() const override {
    return settings_;
  }
  SequencerLocator& getSequencerLocator() const override {
    return *locator_;
  }
  ClusterState* getClusterState() const override {
    return cluster_state_;
  }

  Settings settings_;
  void startClusterStateRefreshTimer() override {}

 private:
  std::shared_ptr<const NodesConfiguration> nodes_config_;
  std::shared_ptr<SequencerLocator> locator_;
  ClusterState* cluster_state_;
};

class MockHashBasedSequencerLocator : public HashBasedSequencerLocator {
 public:
  MockHashBasedSequencerLocator(
      ClusterState* cluster_state,
      std::shared_ptr<const Configuration> config,
      Settings settings = create_default_settings<Settings>())
      : HashBasedSequencerLocator(),
        settings_(settings),
        cluster_state_(cluster_state),
        config_(config) {}

  ~MockHashBasedSequencerLocator() override {}

  ClusterState* getClusterState() const override {
    return cluster_state_;
  }

  std::shared_ptr<const Configuration> getConfig() const override {
    return config_;
  }

  const Settings& getSettings() const override {
    return settings_;
  }

  std::shared_ptr<const configuration::nodes::NodesConfiguration>
  getNodesConfiguration() const override {
    return config_->getNodesConfiguration();
  }

 private:
  Settings settings_;
  ClusterState* cluster_state_;
  std::shared_ptr<const Configuration> config_;
};

}} // namespace facebook::logdevice

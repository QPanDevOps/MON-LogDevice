/**
 * Copyright (c) 2017-present, Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */
#pragma once

#include "logdevice/common/ClientID.h"
#include "logdevice/common/Connection.h"
#include "logdevice/common/ConnectionKind.h"
#include "logdevice/common/Request.h"
#include "logdevice/common/RequestType.h"
#include "logdevice/common/ResourceBudget.h"
#include "logdevice/common/Sockaddr.h"

namespace facebook { namespace logdevice {

/**
 * @file Created by ConnectionListener when a new incoming connection (from a
 *       client or another LogDevice server) is accepted.  The worker thread
 *       processing this request assumes ownership of the socket provided by
 *       libevent.
 */

class NewConnectionRequest : public Request {
 public:
  NewConnectionRequest(int fd,
                       worker_id_t worker_id,
                       const Sockaddr& client_addr,
                       ResourceBudget::Token conn_token,
                       ResourceBudget::Token conn_backlog_token,
                       SocketType type,
                       ConnectionType conntype,
                       ConnectionKind connection_kind,
                       WorkerType worker_type = WorkerType::GENERAL)
      : Request(RequestType::NEW_CONNECTION),
        fd_(fd),
        worker_id_(worker_id),
        client_addr_(client_addr),
        conn_token_(std::move(conn_token)),
        conn_backlog_token_(std::move(conn_backlog_token)),
        sock_type_(type),
        conntype_(conntype),
        worker_type_(worker_type),
        connection_kind_(connection_kind) {}

  ~NewConnectionRequest() override {}

  Request::Execution execute() override;

  int getThreadAffinity(int nthreads) override;

  int8_t getExecutorPriority() const override {
    // Assigning similar priority to other sockets events.
    return folly::Executor::MID_PRI;
  }

  WorkerType getWorkerTypeAffinity() override {
    return worker_type_;
  }

  void setConnectionType(ConnectionType conntype) {
    conntype_ = conntype;
  }

 private:
  const int fd_;
  const worker_id_t worker_id_;
  const Sockaddr client_addr_;
  ResourceBudget::Token conn_token_;
  ResourceBudget::Token conn_backlog_token_;
  SocketType sock_type_{SocketType::DATA};
  ConnectionType conntype_;
  // New connections on this listener will be routed to this worker type
  WorkerType worker_type_{WorkerType::GENERAL};
  ConnectionKind connection_kind_;
};

}} // namespace facebook::logdevice

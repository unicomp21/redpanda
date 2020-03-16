#pragma once
#include "cluster/namespace.h"
#include "cluster/partition_manager.h"
#include "config/configuration.h"
#include "kafka/errors.h"
#include "kafka/groups/group.h"
#include "kafka/requests/heartbeat_request.h"
#include "kafka/requests/join_group_request.h"
#include "kafka/requests/leave_group_request.h"
#include "kafka/requests/offset_commit_request.h"
#include "kafka/requests/offset_fetch_request.h"
#include "kafka/requests/sync_group_request.h"
#include "seastarx.h"

#include <seastar/core/future.hh>
#include <seastar/core/sharded.hh>

#include <absl/container/flat_hash_map.h>
#include <cluster/partition_manager.h>

namespace kafka {

/// \brief Manages the Kafka group lifecycle.
class group_manager {
public:
    group_manager(
      ss::sharded<cluster::partition_manager>& pm, config::configuration& conf)
      : _pm(pm)
      , _conf(conf) {}

    ss::future<> start();
    ss::future<> stop();

public:
    /// \brief Handle a JoinGroup request
    ss::future<join_group_response> join_group(join_group_request&& request);

    /// \brief Handle a SyncGroup request
    ss::future<sync_group_response> sync_group(sync_group_request&& request);

    /// \brief Handle a Heartbeat request
    ss::future<heartbeat_response> heartbeat(heartbeat_request&& request);

    /// \brief Handle a LeaveGroup request
    ss::future<leave_group_response> leave_group(leave_group_request&& request);

    /// \brief Handle a OffsetCommit request
    ss::future<offset_commit_response>
    offset_commit(offset_commit_request&& request);

    /// \brief Handle a OffsetFetch request
    ss::future<offset_fetch_response>
    offset_fetch(offset_fetch_request&& request);

public:
    static error_code validate_group_status(const group_id& group, api_key api);
    static bool valid_group_id(const group_id& group, api_key api);

private:
    group_ptr get_group(const group_id& group) {
        if (auto it = _groups.find(group); it != _groups.end()) {
            return it->second;
        }
        return nullptr;
    }

    cluster::notification_id_type _manage_notify_handle;
    ss::gate _gate;

    void attach_partition(ss::lw_shared_ptr<cluster::partition>);

    struct attached_partition {
        bool loading;
        ss::lw_shared_ptr<cluster::partition> partition;

        attached_partition(ss::lw_shared_ptr<cluster::partition> p)
          : loading(true)
          , partition(std::move(p)) {}
    };

    absl::flat_hash_map<model::ntp, ss::lw_shared_ptr<attached_partition>>
      _partitions;

    ss::sharded<cluster::partition_manager>& _pm;
    config::configuration& _conf;
    absl::flat_hash_map<group_id, group_ptr> _groups;
};

} // namespace kafka

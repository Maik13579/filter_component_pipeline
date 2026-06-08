# filter_component_synchronizer

`filter_component_synchronizer` contains a header-only synchronizer for components
that consume more than one adapted input stream while preserving
`std::unique_ptr` ownership.

The synchronizer owns one typed subscription per input port. It stores unmatched
messages by port name, such as `input_1` and `input_2`, and invokes a ready
callback when a complete input set is available. Component authors then access
the synchronized messages through typed port accessors rather than raw
type-erased storage.

The default mode is `receipt_time`. Each subscription callback stamps the input
with `node.get_clock()->now()`, so message headers are ignored. When `use_sim_time`
is enabled, synchronization follows the node's ROS time source.

Multi-input components support:

- `sync.queue_size`: number of unmatched messages retained per input port.
- `sync.max_interval`: maximum receipt-time span across a selected input set.
- `sync.mode`: `receipt_time` or `latest`.

`latest` mode skips bounded queue matching and fires whenever any input updates
after every port has received at least one message. Accessors return the most
recent data for each port.

These settings are meaningful only for components with more than one input port,
such as `PointCloudMerger`.

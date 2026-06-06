# pcl_filter_synchronizer

`pcl_filter_synchronizer` contains a header-only synchronizer for components
that consume more than one adapted input stream while preserving
`std::unique_ptr` ownership.

The synchronizer owns one typed subscription per input port. It stores unmatched
messages by port name, such as `input_1` and `input_2`, and invokes a ready
callback when a complete input set is available. Component authors then access
the synchronized messages through typed port accessors rather than raw
type-erased storage.

The supported policies are `ExactTime` and `ApproximateTime`. The corresponding
component parameters are:

- `sync.policy`: matching policy.
- `sync.queue_size`: number of unmatched messages retained per input port.
- `sync.slop`: timestamp tolerance for approximate matching.

These settings are meaningful only for components with more than one input port,
such as `PointCloudMerger`.

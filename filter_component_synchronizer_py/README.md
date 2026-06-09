# filter_component_synchronizer_py

Python synchronizer for filter component inputs.

This package mirrors the C++ `filter_component_synchronizer` package for Python
filters. It stores messages by named input port and calls a ready callback when a
complete input set is available.

## Features

- Single-input processing on every received message.
- Multi-input `receipt_time` synchronization.
- Multi-input `latest` synchronization.
- Configurable `queue_size` and `max_interval`.
- Type checks by adapter class per input port.

## Basic Usage

```python
from filter_component_synchronizer_py import FilterComponentSynchronizer
from filter_component_synchronizer_py import SynchronizerOptions


sync = FilterComponentSynchronizer(
    SynchronizerOptions(mode="receipt_time", queue_size=10, max_interval=0.05),
    ready_callback=lambda: process_inputs(),
)
sync.add_input("cloud", PointCloud2NumpyAdapter)
sync.store_message("cloud", PointCloud2NumpyAdapter, cloud, receipt_stamp_ns)
cloud = sync.take_input("cloud", PointCloud2NumpyAdapter)
```

Most users should access this through `filter_component_base_py` instead of
constructing it directly.

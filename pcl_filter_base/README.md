# pcl_filter_base

`pcl_filter_base` contains the reusable lifecycle component infrastructure for
PCL filter components. It is header-only and uses the
`pcl_filter_base::ros` namespace.

The central idea is that a component declares port descriptors. Those
descriptors are the source of the component's subscriptions, publishers, topic
parameters, QoS parameters, and synchronization behavior.

## Port Descriptors

Each input or output descriptor has a port name, a default topic, an adapter
type, and help text. During lifecycle configuration, the base class uses input
descriptors to declare parameters such as `inputs.cloud.topic` and
`inputs.cloud.qos.reliability`, then creates the matching typed subscriptions.
It uses output descriptors in the same way for parameters such as
`outputs.cloud.topic` and `outputs.cloud.qos.depth`, then creates typed
publishers.

Derived components receive data by named port:

```cpp
auto cloud = takeInput<CloudAdapter>("cloud");
```

They publish results through named output ports:

```cpp
publish<CloudAdapter>("cloud", filtered_cloud);
```

The port name in code is the same port name that appears in editor edges and
saved YAML.

## Synchronization

Components with one input port process each incoming message independently.
Components with more than one input port get synchronization parameters:

- `sync.policy`: `ExactTime` or `ApproximateTime`.
- `sync.queue_size`: how many unmatched messages to retain per input port.
- `sync.slop`: timestamp tolerance for approximate matching.

These parameters exist only for multi-input components. A complete synchronized
input set is then available through the same `takeInput<AdapterT>("port")`
accessors used by single-input components.

## Creating a Custom Component

Custom components derive from `PclFilterComponentBase<PointT, FilterT>` and
pass compile-time input and output descriptor arrays into the base constructor.
The constructor call names the component and gives the base enough information
to declare port topics, QoS parameters, publishers, subscriptions, and optional
sync parameters.

`inputPorts()` declares what the component consumes. `outputPorts()` declares
what it can publish. `configureFilter()` reads filter-specific parameters and
configures the wrapped PCL filter object before the component processes data.

```cpp
template <typename PointT>
class MyComponent
  : public pcl_filter_base::ros::PclFilterComponentBase<PointT, MyFilter<PointT>>
{
public:
  using Base = pcl_filter_base::ros::PclFilterComponentBase<PointT, MyFilter<PointT>>;
  using CloudAdapter = typename Base::CloudAdapter;
  using PortDescriptor = typename Base::PortDescriptor;

  explicit MyComponent(const rclcpp::NodeOptions & options)
  : Base("my_filter", options, inputPorts(), outputPorts())
  {
    // Declare filter-specific parameters here.
  }

protected:
  static std::array<PortDescriptor, 1> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>(
        "cloud",
        "/points/input",
        "Input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 1> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>(
        "cloud",
        "/points/output",
        "Filtered point cloud topic."),
    }};
  }

  void configureFilter() override
  {
    // Read filter-specific parameters and configure this->filter_.
  }
};
```

For multiple inputs, add more entries to `inputPorts()`. The base declares sync
parameters automatically when the input descriptor array has more than one
entry. For multiple outputs, declare each output with the adapter type that
matches the message it publishes, such as a cloud adapter for `cloud` and a
point-indices adapter for `indices`.

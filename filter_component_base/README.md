# filter_component_base

`filter_component_base` contains the reusable lifecycle component infrastructure for
ROS filter components. It is header-only and uses the
`filter_component_base::ros` namespace.

The central idea is that a component declares port descriptors. Those
descriptors are the source of the component's subscriptions, publishers, topic
parameters, QoS parameters, and synchronization behavior.

## Port Descriptors

Each input or output descriptor has a port name, an adapter type, and help text.
During lifecycle configuration, the base class uses input descriptors to declare
parameters such as `inputs.cloud.topic` and `inputs.cloud.qos.reliability`, then
creates the matching typed subscriptions. It uses output descriptors in the same
way for parameters such as `outputs.cloud.topic` and
`outputs.cloud.qos.depth`, then creates typed publishers.

Topic parameters are declared automatically from the port descriptors. Input
ports use `~/_input/<port_name>` and output ports use
`~/_output/<port_name>` as their generated defaults; component authors do not
set these defaults in the port descriptor. In saved graph workflows,
`filter_component_factory` maps graph edges onto those parameters when it
launches the component pipeline.

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

Custom components derive from `FilterComponentBase` and
pass compile-time input and output descriptor arrays into the base constructor.
The constructor call names the component and gives the base enough information
to declare topic parameters, QoS parameters, publishers, subscriptions, and
optional sync parameters.

`inputPorts()` declares what the component consumes. `outputPorts()` declares
what it can publish. `configure()` reads component-specific parameters and
configures any algorithm state before the component processes data.

```cpp
template <typename PointT>
class MyComponent
  : public filter_component_base::ros::FilterComponentBase
{
public:
  using Base = filter_component_base::ros::FilterComponentBase;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;
  using StampedCloud = pcl::PointCloud<PointT>;
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
        "Input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 1> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>(
        "cloud",
        "Filtered point cloud topic."),
    }};
  }

  void configure() override
  {
    filter_.configure(readParams());
  }

  void process() override
  {
    auto input = this->template takeInput<CloudAdapter>("cloud");
    auto output = std::make_unique<StampedCloud>();
    output->header = input->header;
    filter_.filter(*input, *output);
    this->template publish<CloudAdapter>("cloud", std::move(output));
  }

  MyFilter<PointT> filter_;
};
```

For multiple inputs, add more entries to `inputPorts()`. The base declares sync
parameters automatically when the input descriptor array has more than one
entry. For multiple outputs, declare each output with the adapter type that
matches the message it publishes, such as a cloud adapter for `cloud` and a
point-indices adapter for `indices`.

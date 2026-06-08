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
the matching topic and QoS parameters, then creates typed subscriptions. It uses
output descriptors in the same way to create typed publishers.

Topic parameters are declared automatically from the port descriptors. Component
authors declare ports; saved graph workflows assign the topic bindings when
`filter_component_factory` launches the component pipeline.

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

- `sync.mode`: `receipt_time` or `latest`.
- `sync.queue_size`: how many unmatched messages to retain per input port.
- `sync.max_interval`: maximum receipt-time span across a synchronized input set.

Receipt-time synchronization uses `node.get_clock()->now()` in the subscription
callback and ignores message/header stamps. Nodes using ROS time, including sim
time, synchronize against that node clock. `latest` mode fires whenever any input
updates after all ports have a latest message. These parameters exist only for
multi-input components. A complete input set is then available through the same
`takeInput<AdapterT>("port")` accessors used by single-input components.

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

## Wrapping ROS Filter Chains

`filter_component_base::ros::FilterChainComponent<AdapterT, TraitsT>` wraps the
upstream [`ros/filters`](https://github.com/ros/filters) `filters::FilterChain<T>`
API as a single-input, single-output lifecycle component. The ROS boundary still
uses the typed adapter and `std::unique_ptr` messages. The chain itself uses the
official value-based `update(const T &, T &)` API, so values may be copied during
chain processing.

`AdapterT` is any `rclcpp::TypeAdapter` type used by the component ports.
`TraitsT` supplies the component contract:

```cpp
struct MyChainTraits
{
  static const char * nodeName() {return "my_filter_chain";}
  static const char * dataType() {return "pcl::PointCloud<pcl::PointXYZ>";}
  static const char * inputPort() {return "cloud";}
  static const char * outputPort() {return "cloud";}
  static const char * defaultParamPrefix() {return "filter_chain";}
};
```

The component declares `filter_chain.param_prefix`, defaulting to
`TraitsT::defaultParamPrefix()`, and passes that prefix with the node logging and
parameter interfaces to `filters::FilterChain<T>::configure()`. Chain plugins
must be exported for the exact `filters::FilterBase<T>` base class string named
by `TraitsT::dataType()`.

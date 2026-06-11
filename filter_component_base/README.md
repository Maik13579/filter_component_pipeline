# filter_component_base

`filter_component_base` contains the reusable lifecycle component infrastructure for
ROS filter components. It is header-only and uses the
`filter_component_base::ros` namespace. Descriptor types live in
`filter_component_base/ros/component_descriptors.hpp`; the base lifecycle
behavior lives in `filter_component_base/ros/filter_component_base.hpp`.

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

`FilterComponentBase` keeps compatibility aliases such as
`FilterComponentBase::PortDescriptor`, but code that only needs the descriptor
types can include `filter_component_base/ros/component_descriptors.hpp`.

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

## Shared-Memory Keys

Components can declare typed shared-memory keys in addition to input and output
ports. The base class uses `component_shm::ShmView` internally and declares one
string remap parameter per key:

```text
shm_key.<key_name>
```

The default remap value is the key name itself. During each lifecycle configure,
the base reads all `shm_key.*` values and installs those remaps into the view.
Saved graph YAML can bind a local key such as `global_map` to an effective
process-wide key such as `slam/global_map`.

Filter authors should use the checked helper API instead of accessing the
registry directly:

```cpp
auto map = shmGet<MyMap>("global_map");
auto maybe_cache = shmTryGet<PoseCache>("pose_cache");
auto mutable_map = shmGetMutable<MyMap>("global_map");
shmSet<MyMap>("global_map", next_map);
shmSetShared<MyMap>("global_map", shared_map);
```

All helpers verify that the key was declared and that the requested C++ type
matches the declared key type. Mutable access and replacement helpers also
require `ShmAccess::ReadWrite`; read-only keys can only use const get helpers.
This prevents accidental writes through the filter author API, although it
cannot prevent mutation through a mutable shared pointer obtained elsewhere.

## Creating a Custom Component

Custom components derive from `FilterComponentBase` and
pass compile-time input and output descriptor arrays into the base constructor.
The constructor call names the component and gives the base enough information
to declare topic parameters, QoS parameters, publishers, subscriptions, and
optional sync parameters.

`inputPorts()` declares what the component consumes. `outputPorts()` declares
what it can publish. `configure()` reads component-specific parameters and
configures any algorithm state before the component processes data.
Filter-specific parameters are usually declared in the constructor with
`declareParameter(name, default_value, description)` and read during
`configure()` with `getParameter<T>(name)`.

```cpp
template <typename PointT>
class MyComponent
  : public filter_component_base::ros::FilterComponentBase
{
public:
  using Base = filter_component_base::ros::FilterComponentBase;
  using CloudAdapter = custom_components::ros::CloudAdapter<PointT>;
  using StampedCloud = custom_components::Cloud<PointT>;
  using PortDescriptor = typename Base::PortDescriptor;
  using ShmKeyDescriptor = typename Base::ShmKeyDescriptor;

  explicit MyComponent(const rclcpp::NodeOptions & options)
  : Base("my_filter", options, inputPorts(), outputPorts(), shmKeys())
  {
    this->declareParameter(
      "filter.scale",
      1.0,
      "Scale applied to the filtered cloud.");
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

  static std::array<ShmKeyDescriptor, 1> shmKeys()
  {
    return {{
      Base::template shmKey<MyMap>(
        "global_map",
        "Shared map used by this filter.",
        Base::ShmAccess::ReadWrite,
        "my_pkg::MyMap"),
    }};
  }

  void configure() override
  {
    typename MyFilter<PointT>::Params params;
    params.scale = this->template getParameter<double>("filter.scale");
    filter_.configure(params);
  }

  void process() override
  {
    auto input = this->template takeInput<CloudAdapter>("cloud");
    auto shared_map = this->template shmGetMutable<MyMap>("global_map");
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

## Inspecting Component Metadata

The package installs an inspector tool that loads exported C++ filter components,
constructs each lifecycle node without configuring or activating it, and prints
JSON metadata read from `FilterComponentBase`:

```bash
ros2 run filter_component_base inspect_filter_components --packages-select my_filters
ros2 run filter_component_base inspect_filter_components my_filters
ros2 run filter_component_base inspect_filter_components --all
```

Use `-h` or `--help` for the full CLI. The tool reads package XML only to find
which filters a package exports; port descriptors, shared-memory key descriptors,
and parameter descriptors come from the component instance itself.

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
  static const char * originalInputPort() {return "orig_input";}
};
```

The component publishes the filtered result on `outputPort()` and the unchanged
input message on `originalInputPort()`.

The component passes the node logging and parameter interfaces to
`filters::FilterChain<T>::configure()`. Chain plugins must be exported for the
exact `filters::FilterBase<T>` base class string named by `TraitsT::dataType()`.

# PCL Packages

This directory groups the PCL-specific packages. The generic filter framework
packages stay at the repository root so future image, OpenCV, or other message
component families can be added alongside `pcl/`.

The PCL packages provide one concrete component family for the generic filter
component framework. They export PCL logical types and filters through the same
`<filter_component>` metadata used by the editor and factory.

## Packages

- [pcl_filter_components_type_adapters](pcl_filter_components_type_adapters/README.md): PCL cloud and
  point-indices type adapters plus logical type discovery metadata.
- [pcl_filter_components](pcl_filter_components/README.md): generic PCL filter
  algorithms and reusable component templates.
- [pcl_filter_components_xyz](pcl_filter_components_xyz/README.md): `PointXYZ` aliases, filter
  metadata, and registered components.
- [pcl_filter_components_xyzi](pcl_filter_components_xyzi/README.md): `PointXYZI` aliases, filter
  metadata, and registered components.
- [pcl_filter_components_xyzrgb](pcl_filter_components_xyzrgb/README.md): `PointXYZRGB` aliases,
  filter metadata, and registered components.
- [pcl_filter_components_xyzrgba](pcl_filter_components_xyzrgba/README.md): `PointXYZRGBA` aliases,
  filter metadata, and registered components.
- [pcl_filter_components_tests](pcl_filter_components_tests/README.md): repository validation coverage
  for discovery, registration, and example graph parsing.

## Architecture

```text
pcl_filter_components_type_adapters
  -> pcl_filter_components
    -> pcl_filter_components_xyz / pcl_filter_components_xyzi / pcl_filter_components_xyzrgb / pcl_filter_components_xyzrgba
      -> pcl_filter_components_tests
```

The PCL packages depend on the generic `filter_component_base` and
`filter_component_synchronizer` packages from the repository root. The editor
and factory discover PCL components through the generic `<filter_component>`
package metadata tag.

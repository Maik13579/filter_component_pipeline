# filter_component_python_examples

Example Python filters for the filter component pipeline framework.

The package exports `PythonPointCloudPassthrough` with the
`<filter_component_py>` package metadata so it appears in
`filter_component_editor`.

## Filter

`PythonPointCloudPassthrough` uses `PointCloud2NumpyAdapter` and republishes the
incoming point cloud unchanged. It is intentionally simple and exists as a
working template for Python filter prototypes.

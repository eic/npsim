\defgroup Geometry_cad CAD converters
\ingroup Geometry
\brief Classes to convert geometries to CAD systems

A collection of classes convection ROOT geometry to CAD systems:

  - OpenCascade
  - STEP.

`TOCCToStep` uses OpenCascade XCAF metadata so exported STEP can carry shape names,
volume display color/transparency, and material metadata (material name + density).

A power curve is identified par its Date and defined by its array of points (Time, Power).

Given a coclustering model, preparing a deployment model allow to assign new power curves to their closest cluster.
For each deployed curve, new variables can be added, such as:
  - cluster of curve (if curve belongs to train dataset)
  - predicted cluster of curves
  - the distance to all other (date) clusters of power curves
  - the number of points on all other time or power intervals
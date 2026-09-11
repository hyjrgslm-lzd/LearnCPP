# B02_bulk_domain: bulk granularity and domain lowering

This is an observation unit. It shows two real paths that have the same output:

- the default path uses `just(7) | then(square)`;
- the custom path returns a sender whose completion domain lowers `set_value`
  through `transform_sender` into `just(49)`.

The point is the protocol boundary: domain lowering happens during connection,
before the operation state starts. The `bulk` check is separate and records
logical item count. With `seq` the logical work runs inline; a scheduler or
implementation may later map the same logical shape to different execution
locations.

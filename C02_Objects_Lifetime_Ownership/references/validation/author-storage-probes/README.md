# C02 storage and frontier capability probes

These probes record local toolchain behavior for C02 chapters 11-15. They are evidence, not course text.

Run from the repository root:

```powershell
python Core_Study/references/validation/author-storage-probes/run_probes.py
```

Outputs are written under `build/c02-storage-probes/evidence/` and never overwrite an existing JSON file.

Coverage:

- `env_report`: compiler identity, standard mode, `_MSVC_LANG`, `__cplusplus`, and relevant feature-test macros.
- `implicit_move`: C++23 named-local implicit move and move-only return behavior.
- `range_for_lifetime_macro`: `__cpp_range_based_for` only; no dangling range is executed.
- `start_lifetime_as`: actual `<memory>` `std::start_lifetime_as*` compile/link/run when provided by the STL.
- `frontier_base_designated`: C++26/29 base-class designated initialization syntax probe.
- `frontier_return_temp_ref`: compile-only probe for future rejection of returning a temporary as a reference.

Provenance, invalid-pointer, lifetime-end DRs, erroneous initialization behavior, and defaulted assignment wording do not have a reliable safe runtime probe here. Keep those as standard-text notes until a direct compiler diagnostic or safe model is chosen.

$ErrorActionPreference = 'Stop'
rg --glob '!**/build/**' '#include "(lifetime_model|memory_model)\.hpp"' Core_Study/exercises/L03_lifetimes/src Core_Study/exercises/L03_lifetimes/reference Core_Study/exercises/L03_lifetimes/validation Core_Study/exercises/L05_special_members/src Core_Study/exercises/L05_special_members/reference Core_Study/exercises/L05_special_members/validation
if ($LASTEXITCODE -eq 1) {
    Write-Output 'implementation trusted-header bare include scan: no matches'
} elseif ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
rg --glob '!**/build/**' '#include "support/(lifetime_model|memory_model)\.hpp"' Core_Study/exercises/L03_lifetimes/src Core_Study/exercises/L03_lifetimes/reference Core_Study/exercises/L03_lifetimes/validation Core_Study/exercises/L05_special_members/src Core_Study/exercises/L05_special_members/reference Core_Study/exercises/L05_special_members/validation


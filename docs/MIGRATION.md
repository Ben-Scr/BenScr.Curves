# Initial release

Created by Ben Schneider — https://github.com/Ben-Scr

BenScr.Curves 1.0.0 is a new independent C++ library. No previous package IDs, public APIs, or
private application source are migrated. Existing BenScr packages are unchanged.

- NuGet ID and repository: `BenScr.Curves`.
- CMake package: `BenScrCurves`; target: `BenScr::Curves`.
- Header: `benscr/curves/curves.hpp`; namespace: `BenScr::Curves`.
- C++20 is required. The package is header-only and does not ship a managed API or native ABI.

Applications adopting it must choose their time units, key ordering, and interpolation functions
explicitly. Bezier progress is time-mapped, not the curve parameter. Inputs outside the progress
interval clamp, rather than using CSS tangent extrapolation. Track views are borrowed; tracks
own their copied keys. These are the initial API contracts, not compatibility shims for another API.

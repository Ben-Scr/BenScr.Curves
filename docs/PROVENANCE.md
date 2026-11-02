# Provenance

Created by Ben Schneider — https://github.com/Ben-Scr

The easing formulas, cubic inversion and keyframe implementation are new independent source.
No private application source or third-party implementation has been copied. The cubic control
convention and CSS ease reference controls are described by the
[W3C CSS Easing Functions specification](https://www.w3.org/TR/css-easing-1/#cubic-bezier-easing-functions).
This library adopts its normalized Bezier interpretation, not the complete CSS behavior.

Build, native packaging and verification infrastructure adapt the existing BenScr.Binary tools
under the same author license. Domain source, examples, tests and API documents are specific to
this library. Tests compare known analytic cases and direct Bernstein polynomials evaluated at
known parameters against the time-inverting implementation; fixed seeds make the corpus repeatable.

The unchanged custom `LICENSE.txt` preserves its original UTF-8/CRLF bytes, SHA-256:
`f6600e285e17c48188a65745d7595807e1755c72201e8eef201a9174fca18573`.

`branding/BenScr_Icon_Master.png` is copied unchanged and embedded as `icon.png`. It is a
1254 x 1254 RGBA PNG, 795819 bytes, SHA-256:
`a5dcabceb766e13c1840a95130ee98a458e12e3713fd9b07f3ceae75a1f5b993`.
No resizing, redesign, or substitute family icon is used. This is the same exact asset used by the
existing BenScr packages; package auditing checks its bytes. Gallery rendering is verified during
publication, separately from this source/package check.

The exact target GitHub repository returned 404 before creation. Package metadata and the
verification manifest identify the eventual source commit, archive hash and release tag.

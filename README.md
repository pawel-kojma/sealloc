# Security enchanced allocator

Prototype `malloc(3)` implementation which aims to improve security.
Key features are:
- Separation of allocator metadata and user data to avoid leakage.
- Double-free resistance.
- Linear overflow resistance using guard pages.
- Protection from freeing fake region
- Use-after-free protection using randomized region allocation.

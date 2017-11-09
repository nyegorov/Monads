========================================================================
    CONSOLE APPLICATION : [!output PROJECT_NAME] Project Overview
========================================================================

C++ Implementation of some thoughts about Monads, FOP, etc.

////////////////////////////////////////////////////////////////////////////

Overrides '|' operator to perform chaining of computations in Haskell style.

Example: ints() | square | take(5) | filter(even) | sum() // = 20

Defines monadic interface for std::optional, std::list and std::generator

/////////////////////////////////////////////////////////////////////////////

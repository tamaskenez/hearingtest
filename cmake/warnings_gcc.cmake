if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(
		-Werror
		-pedantic                   # Strict compliance to the standard is not met.
		-Wall                       # Enables most warnings.
		-Warith-conversion          # Stricter implicit conversion warnings in arithmetic operations.
		-Wc++11-extensions
		-Wcast-align                # Pointer casts which increase alignment.
		# -Wcast-qual # JUCE                # A pointer is cast to remove a type qualifier, or add an unsafe one.
		-Wconversion                # Implicit type conversions that may change a value.
		# -Wdate-time # JUCE                # Warn when encountering macros that might prevent bit-wise-identical compilations.
		-Wdeprecated
		-Wdisabled-optimization     # GCC’s optimizers are unable to handle the code effectively.
		-Wduplicated-branches       # Warn about duplicated branches in if-else statements.
		-Wduplicated-cond           # Warn about duplicated conditions in an if-else-if chain.
		-Weffc++                    # Warnings related to guidelines from Scott Meyers’ Effective C++ books.
		-Wextra                     # Enables an extra set of warnings.
		-Wextra-semi                # Redundant semicolons after in-class function definitions.
		-Wformat=2                  # printf/scanf/strftime/strfmon format string anomalies.
		# -Wimplicit-fallthrough # JUCE
		-Wlogical-op                # Warn when a logical operator is always evaluating to true or false.
		-Wmisleading-indentation    # Warn when indentation does not reflect the block structure.
		-Wnon-virtual-dtor          # Non-virtual destructors are found.
		-Wnull-dereference          # Dereferencing a pointer may lead to undefined behavior.
		# -Wold-style-cast # JUCE           # C-style cast is used in a program.
		-Woverloaded-virtual        # Overloaded virtual function names.
		-Wredundant-decls           # Something is declared more than once in the same scope.
		-Wredundant-tags            # Redundant class-key and enum-key where it can be eliminated.
		-Wshadow                    # One variable shadows another.
		-Wsign-conversion           # Implicit conversions between signed and unsigned integers.
		-Wsign-promo                # Overload resolution chooses a promotion from unsigned to a signed type.
		-Wsuggest-final-methods     # Virtual methods that could be declared final or in an anonymous namespace.
		-Wsuggest-final-types       # Types with virtual methods that can be declared final or in an anonymous namespace.
		-Wsuggest-override          # Overriding virtual functions that are not marked with the override keyword.
		-Wswitch-enum               # A switch statement has an index of enumerated type and lacks a case.
		# -Wundef # JUCE                    # An undefined identifier is evaluated in an #if directive.
		-Wuninitialized
		-Wunreachable-code
		-Wunsafe-loop-optimizations # The loop cannot be optimized because the compiler cannot assume anything.
		-Wunused                    # Enable all -Wunused- warnings.
		-Wuseless-cast              # Warn about useless casts.
	)
endif()

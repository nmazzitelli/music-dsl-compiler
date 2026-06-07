#ifndef COMPILER_STATE_HEADER
#define COMPILER_STATE_HEADER

#ifndef MUSIC_MODEL_HEADER
typedef struct MusicComposition MusicComposition;
#endif

/**
 * The global state of the compiler. Should transport every data structure
 * needed across the different phases of a compilation.
 */
typedef struct {
	/**
	 * The root node of the AST.
	 */
	void * abstractSyntaxtTree;

	/**
	 * Lowered music model used by the backend.
	 */
	MusicComposition * musicComposition;

	/**
	 * Legacy backend payload. It remains in place while semantic analysis and
	 * IR generation are introduced incrementally in later commits.
	 */
	signed int value;

	// TODO: Add a symbol table.
	// TODO: Add an stack to handle nested scopes.
	// TODO: Add more configuration.
	// TODO: Add whatever you need.
	// TODO: ...
} CompilerState;

#endif

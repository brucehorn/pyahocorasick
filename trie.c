/*
	Procedures that work with trie.

*/

static TrieNode*
trienode_new(char byte, char eow) {
	TrieNode* node = (TrieNode*)memalloc(sizeof(TrieNode));
	if (node) {
		node->output.integer = 0;
		node->fail		= NULL;

		node->n		= 0;
		node->byte	= byte;
		node->eow	= eow;
		node->next	= NULL;
		node->hasoutput = 0;
	}

	return node;
}


static TrieNode* PURE ALWAYS_INLINE
trienode_get_next(TrieNode* node, const uint8_t byte) {
	ASSERT(node);
	if (node->n == 256)
		return node->next[byte];
	else {
		int i;
		for (i=0; i < node->n; i++)
			if ((node)->next[i]->byte == byte)
				return node->next[i];

		return NULL;
	}
}

int
trienode_sort_cmp(const void* a, const void* b) {
#define A ((TrieNode*)a)
#define B ((TrieNode*)b)
	return (A->byte > B->byte) - (A->byte < B->byte);
#undef A
#undef B
}


static TrieNode*
trienode_set_next(TrieNode* node, const uint8_t byte, TrieNode* child) {
	ASSERT(node);
	ASSERT(child);
	ASSERT(trienode_get_next(node, byte) == NULL);

	const int n = node->n;
	TrieNode** next = (TrieNode**)memrealloc(node->next, (n + 1) * sizeof(TrieNode*));
	if (next) {
		node->next = next;
		node->next[n] = child;
		node->n += 1;

		if (node->n == 256) {
			qsort(node->next, 256, sizeof(TrieNode*), trienode_sort_cmp);
		}
		
		return child;
	}
	else
		return NULL;
}


static TrieNode*
trie_add_byte(TrieNode* node, const uint8_t byte) {
	TrieNode* child;
	child = trienode_get_next(node, byte);
	if (child == NULL) {
		child = trienode_new(byte, false);
		if (child)
			trienode_set_next(node, byte, child);
		else
			return NULL;
	}

	return child;
}


static TrieNode*
trie_add_word(Automaton* automaton, char* word, size_t wordlen, bool* new_word) {
	if (automaton->kind == EMPTY) {
		ASSERT(automaton->root == NULL);
		automaton->root = trienode_new('\0', false);
	}

	TrieNode* node = automaton->root;
	TrieNode* child;

	int i;
	for (i=0; i < wordlen; i++) {
		child = trie_add_byte(node, word[i]);
		if (child)
			node = child;
		else
			return NULL;
	}

	if (node->eow == false) {
		node->eow = true;
		*new_word = true;
		automaton->count += 1;
	}
	else
		*new_word = false;
	
	automaton->kind = TRIE;

	return node;
}


static TrieNode*
trie_add_word_UCS2(Automaton* automaton, uint16_t* word, size_t wordlen, bool* new_word) {
	if (automaton->kind == EMPTY) {
		ASSERT(automaton->root == NULL);
		automaton->root = trienode_new('\0', false);
	}

	TrieNode* node = automaton->root;
	TrieNode* child;

	int i;
	for (i=0; i < wordlen; i++) {
		const uint16_t w = word[i];

		child = trie_add_byte(node, w & 0xff);
		if (child)
			node = child;
		else
			return NULL;

		if (w < 0x0100) {
			child = trie_add_byte(node, (w >> 8) & 0xff);
			if (child)
				node = child;
			else
				return NULL;
		}
	}

	if (node->eow == false) {
		node->eow = true;
		*new_word = true;
		automaton->count += 1;
	}
	else
		*new_word = false;
	
	automaton->kind = TRIE;

	return node;
}


static TrieNode*
trie_add_word_UCS4(Automaton* automaton, uint32_t* word, size_t wordlen, bool* new_word) {
	if (automaton->kind == EMPTY) {
		ASSERT(automaton->root == NULL);
		automaton->root = trienode_new('\0', false);
	}

	TrieNode* node = automaton->root;
	TrieNode* child;

	int i;
	for (i=0; i < wordlen; i++) {
#define ADD_BYTE(byte) \
		child = trie_add_byte(node, (byte)); \
		if (child) \
			node = child; \
		else \
			return NULL;

		const uint32_t w = word[i];

		ADD_BYTE(w & 0xff);
		if (w > 0x000000ff) {
			ADD_BYTE((w >> 8) & 0xff);

			if (w > 0x0000ffff) {
				ADD_BYTE((w >> 16) & 0xff);

				if (w > 0x00ffffff) {
					ADD_BYTE((w >> 24) & 0xff);
				}
			}
		}
#undef ADD_BYTE
	}

	if (node->eow == false) {
		node->eow = true;
		*new_word = true;
		automaton->count += 1;
	}
	else
		*new_word = false;
	
	automaton->kind = TRIE;

	return node;
}


static TrieNode* PURE
trie_find(TrieNode* root, const char* word, const size_t wordlen) {
	TrieNode* node;

	node = root;
	ssize_t i;
	for (i=0; i < wordlen; i++) {
		node = trienode_get_next(node, word[i]);
		if (node == NULL)
			return NULL;
	}
		
	return node;
}


static TrieNode* PURE
trie_find_UCS2(TrieNode* root, const uint16_t* word, const size_t wordlen) {
	TrieNode* node;

	node = root;
	ssize_t i;
	for (i=0; i < wordlen; i++) {
		const uint16_t w = word[i];
		node = trienode_get_next(node, w & 0xff);
		if (node == NULL)
			return NULL;

		if (w < 0x0100) {
			node = trienode_get_next(node, (w >> 8) & 0xff);
			if (node == NULL)
				return NULL;
		}
	}
		
	return node;
}


static TrieNode* PURE
trie_find_UCS4(TrieNode* root, const uint32_t* word, const size_t wordlen) {
	TrieNode* node;

	node = root;
	ssize_t i;
	for (i=0; i < wordlen; i++) {
#define NEXT(byte) \
		node = trienode_get_next(node, (byte)); \
		if (node == NULL) \
			return NULL;

		uint32_t w = word[i];

		NEXT(w & 0xff);
		if (w > 0x000000ff) {
			NEXT((w >> 8) & 0xff);

			if (w < 0x0000ffff) {
				NEXT((w >> 16) & 0xff);

				if (w > 0x00ffffff) {
					NEXT((w >> 24) & 0xff);
				}
			}
		}
#undef NEXT
	} // for
		
	return node;
}


static TrieNode* PURE
ahocorasick_next(TrieNode* node, TrieNode* root, const uint8_t byte) {
	TrieNode* next = node;
	TrieNode* tmp;

	while (next) {
		tmp = trienode_get_next(next, byte);
		if (tmp)
			// found link
			return tmp;
		else
			// or go back through fail edges
			next = next->fail;
	}

	// or return root node
	return root;
}


size_t PURE ALWAYS_INLINE
trienode_get_size(const TrieNode* node) {
	return sizeof(TrieNode) + node->n * sizeof(TrieNode*);
}

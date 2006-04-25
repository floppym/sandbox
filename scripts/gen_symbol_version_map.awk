BEGIN {
	split(ENVIRON["SYMBOLS"], SYMBOLS);
}

{
	# Unstripped libc's have '.symtab' section as well, and
	# we should stop processing when we hit that
	if ($0 ~ "^Symbol (.*)table '.symtab'")
		nextfile;

	for (x in SYMBOLS) {
		sym_regex = "^" SYMBOLS[x] "(@|$)";
		# On x86, x86_64 and others, $8 is the symbol name, but on
		# alpha, its $10, so rather use $NF, as it should be the
		# last field
		if ($NF ~ sym_regex) {
			split($NF, symbol_array, /@|@@/);

			# Don't add local symbols of versioned libc's
			if (VERSIONED_LIBC && !symbol_array[2])
				continue;

			# Handle non-versioned libc's like uClibc ...
			if (!symbol_array[2])
				symbol_array[2] = "";
			
			# We have a versioned libc
			if (symbol_array[2] && !VERSIONED_LIBC)
				VERSIONED_LIBC = 1;

			ADD = 1;
			# Check that we do not add duplicates
			for (y in PROCESSED_SYMBOLS) {
				if (y == $NF) {
					ADD = 0;
					break;
				}
			}
			
			if (ADD) {
				SYMBOL_LIST[symbol_array[2]] = SYMBOL_LIST[symbol_array[2]] " " symbol_array[1];
				PROCESSED_SYMBOLS[$NF] = $NF;
			}
		}

		sym_regex = "^__" SYMBOLS[x] "(@@|$)";
		if (($5 == "WEAK") && ($NF ~ sym_regex)) {
			split($NF, symbol_array, /@@/);
			
			# Don't add local symbols of versioned libc's
			if (VERSIONED_LIBC && !symbol_array[2])
				continue;

			# Handle non-versioned libc's like uClibc ...
			if (!symbol_array[2])
				symbol_array[2] = "";
			
			# We have a versioned libc
			if (symbol_array[2] && !VERSIONED_LIBC)
				VERSIONED_LIBC = 1;

			ADD = 1;
			# Check that we do not add duplicates
			for (y in PROCESSED_SYMBOLS) {
				if (y == $NF) {
					ADD = 0;
					break;
				}
			}
			
			if (ADD) {
				SYMBOL_LIST[symbol_array[2]] = SYMBOL_LIST[symbol_array[2]] " " symbol_array[1];
				PROCESSED_SYMBOLS[$NF] = $NF;
			}
		}
	}
}

END {
	for (sym_version in SYMBOL_LIST) {
		if (sym_version)
			VERSIONS = VERSIONS " " sym_version;
	}

	# We need the symbol versions sorted alphabetically ...
	if (VERSIONS) {
		split(VERSIONS, VERSION_LIST);
		COUNT = asort(VERSION_LIST);
	} else {
		# Handle non-versioned libc's like uClibc ...
		COUNT = 1;
	}
	
	for (i = 1; i <= COUNT; i++) {
		if (VERSION_LIST[i]) {
			sym_version = VERSION_LIST[i];
			printf("%s {\n", sym_version);
		} else {
			# Handle non-versioned libc's like uClibc ...
			sym_version = "";
			printf("{\n");
		}
		
		printf("  global:\n");
		
		split(SYMBOL_LIST[sym_version], sym_names);
		
		for (x in sym_names) {
			printf("    %s;\n", sym_names[x]);
		}

		if (!old_sym_version) {
			printf("  local:\n");
			printf("    *;\n");
			printf("};\n");
		} else {
			printf("} %s;\n", old_sym_version);
		}
		
		old_sym_version = sym_version;
	}
}

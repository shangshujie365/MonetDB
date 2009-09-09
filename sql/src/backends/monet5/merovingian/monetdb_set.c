/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://monetdb.cwi.nl/Legal/MonetDBLicense-1.1.html
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2009 MonetDB B.V.
 * All Rights Reserved.
 */

typedef enum {
	SET = 0,
	INHERIT
} meroset;

static void
command_set(int argc, char *argv[], meroset type)
{
	char *p;
	char *property = NULL, *value = NULL;
	err e;
	int i;
	int state = 0;
	sabdb *orig, *stats, *w;
	confkeyval *kv, *props = getDefaultProps();

	if (argc >= 1 && argc <= 2) {
		/* print help message for this command */
		command_help(2, &argv[-1]);
		exit(1);
	} else if (argc == 0) {
		exit(2);
	}

	/* time to collect some option flags */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (p = argv[i] + 1; *p != '\0'; p++) {
				switch (*p) {
					case '-':
						if (p[1] == '\0') {
							i = argc;
							break;
						}
					default:
						fprintf(stderr, "%s: unknown option: -%c\n",
								argv[0], *p);
						command_help(2, &argv[-1]);
						exit(1);
					break;
				}
			}
			/* make this option no longer available, for easy use
			 * lateron */
			argv[i] = NULL;
		} else if (property == NULL) {
			/* first non-option is property, rest is database */
			property = argv[i];
			argv[i] = NULL;
			if (type == SET) {
				if ((p = strchr(property, '=')) == NULL) {
					fprintf(stderr, "set: need property=value\n");
					command_help(2, &argv[-1]);
					exit(1);
				}
				*p++ = '\0';
				value = p;
			}
		}
	}

	if (property == NULL) {
		fprintf(stderr, "%s: need a property argument\n", argv[0]);
		command_help(2, &argv[0]);
		exit(1);
	}

	w = NULL;
	orig = NULL;
	for (i = 1; i < argc; i++) {
		if (argv[i] != NULL) {
			if ((e = SABAOTHgetStatus(&stats, argv[i])) != MAL_SUCCEED) {
				fprintf(stderr, "%s: internal error: %s\n", argv[0], e);
				GDKfree(e);
				exit(2);
			}

			if (stats == NULL) {
				fprintf(stderr, "%s: no such database: %s\n",
						argv[0], argv[i]);
				argv[i] = NULL;
			} else {
				if (stats->state == SABdbRunning) {
					fprintf(stderr, "%s: database '%s' is still running, "
							"stop database first\n", argv[0], stats->dbname);
					SABAOTHfreeStatus(&stats);
					state |= 1;
					continue;
				}
				if (orig == NULL) {
					orig = stats;
					w = stats;
				} else {
					w = w->next = stats;
				}
			}
		}
	}

	for (stats = orig; stats != NULL; stats = stats->next) {
		if (strcmp(property, "name") == 0) {
			char name[4069];
			char *res;
			char *out;

			/* special virtual case */
			if (type == INHERIT) {
				fprintf(stderr, "inherit: cannot default to a database name\n");
				state |= 1;
				continue;
			}

			if (value[0] == '\0')
				continue;

			if (mero_running == 0) {
				if ((e = db_rename(stats->dbname, value)) != NULL) {
					fprintf(stderr, "set: %s\n", e);
					free(e);
					state |= 1;
					continue;
				}
			} else {
				snprintf(name, sizeof(name), "name=%s", value);
				out = control_send(&res, mero_control, -1, stats->dbname, name);
				if (out != NULL || strcmp(res, "OK") != 0) {
					res = out == NULL ? res : out;
					fprintf(stderr, "%s: %s\n", argv[0], res);
					state |= 1;
				}
				free(res);
			}
		} else if (strcmp(property, "shared") == 0) {
			char share[4069];
			char *res;
			char *out;

			if (type == INHERIT)
				value = "";

			snprintf(share, sizeof(share), "share=%s", value);
			out = control_send(&res, mero_control, 0, stats->dbname, share);
			if (out != NULL || strcmp(res, "OK") != 0) {
				res = out == NULL ? res : out;
				fprintf(stderr, "%s: %s\n", argv[0], res);
				state |= 1;
			}
			free(res);
		} else {
			char *err;
			readProps(props, stats->path);
			kv = findConfKey(props, property);
			if (kv == NULL) {
				fprintf(stderr, "%s: no such property: %s\n", argv[0], property);
				state |= 1;
				continue;
			}
			if ((err = setConfVal(kv, type == SET ? value : NULL)) != NULL) {
				fprintf(stderr, "%s: %s\n", argv[0], err);
				GDKfree(err);
				state |= 1;
				continue;
			}

			if (strcmp(kv->key, "forward") == 0 && kv->val != NULL) {
				if (strcmp(kv->val, "proxy") != 0 &&
						strcmp(kv->val, "redirect") != 0)
				{
					fprintf(stderr, "%s: expected 'proxy' or 'redirect' for "
							"property 'forward', got: %s\n", argv[0], kv->val);
					state |= 1;
					continue;
				}
			}

			writeProps(props, stats->path);
			freeConfFile(props);
		}
	}

	if (orig != NULL)
		SABAOTHfreeStatus(&orig);
	GDKfree(props);
	exit(state);
}


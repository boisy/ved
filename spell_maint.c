/* vspell_maint  */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include <ctype.h>
#include <sys/types.h>
#include <malloc.h>
#include <stdio.h>
#include <strings.h>
#include "spell.h"

#define BUFFSIZE 80

#define align(f) if(ftell(f) & 1) putc(0, f)

extern int errno;

char *chars1 = CHAR1;
char *chars2 = CHAR2;

u_int indexes[KEYS1][KEYS2], filepos;

char compword[BUFFSIZE], oldword[BUFFSIZE];
FILE *dictpath, *wordpath;

Smod header;

char *modname = "ved_dict.mod";

u_int dictcount;

#define QUICKMAX 8

char quickwords[][QUICKMAX] =
{
	"a", "able", "about", "above", "account", "across", "act",
	"action", "ad", "added", "after", "again", "against",
	"age", "ago", "ahead", "aid", "air", "all", "almost",
	"alone", "along", "already", "also", "always", "am",
	"among", "amount", "an", "and", "another", "answer", "any",
	"anyone", "appear", "are", "area", "areas", "arms", "army",
	"around", "art", "as", "ask", "asked", "at", "average",
	"away",

	"back", "bad", "ball", "based", "basic", "basis", "be",
	"became", "because", "become", "bed", "been", "before",
	"began", "behind", "being", "believe", "below", "best",
	"better", "between", "beyond", "big", "bill", "black",
	"blood", "blue", "board", "body", "book", "born", "both",
	"boy", "boys", "bring", "brought", "brown", "but", "by",

	"call", "called", "came", "can", "can't", "cannot", "car",
	"care", "carried", "cars", "case", "cases", "cause",
	"cent", "center", "central", "century", "certain",
	"chance", "change", "changes", "charge", "chief", "child",
	"choice", "city", "class", "clear", "clearly", "close",
	"club", "cold", "college", "come", "comes",
	"coming", "common", "company", "control", "corner", "cost",
	"costs", "could", "country", "county", "couple", "course",
	"court", "cut",

	"daily", "dark", "data", "day", "days", "dead", "deal",
	"death", "decided", "deep", "defense", "degree", "design",
	"did", "didn't", "direct", "do", "does", "doing", "don't",
	"done", "door", "doubt", "down", "due", "during",

	"each", "earlier", "early", "earth", "east", "easy",
	"effect", "effects", "effort", "efforts", "either", "else",
	"end", "england", "english", "enough", "entire", "europe",
	"even", "evening", "ever", "every", "example", "except",
	"extent", "eye", "eyes",

	"face", "fact", "faith", "fall", "family", "far", "farm",
	"father", "fear", "federal", "feed", "feel", "feeling",
	"feet", "felt", "few", "field", "figure", "figures",
	"final", "finally", "find", "fine", "fire", "firm",
	"first", "fiscal", "five", "floor", "fo", "food", "for",
	"force", "forces", "foreign", "form", "former", "forms",
	"forward", "found", "four", "free", "freedom", "french",
	"friend", "friends", "from", "front", "full", "further",
	"future",

	"game", "gave", "general", "get", "getting", "girl",
	"girls", "give", "given", "gives", "go", "god", "going",
	"gone", "good", "got", "great", "greater", "green",
	"ground", "group", "groups", "growth", "gun",

	"had", "hair", "half", "hall", "hand", "hands", "hard",
	"has", "have", "having", "he", "he's", "head", "hear",
	"heard", "heart", "heavy", "held", "help", "her", "here",
	"here's", "herself", "high", "higher", "him", "himself",
	"his", "history", "hit", "hold", "home", "hope", "horse",
	"hot", "hotel", "hour", "hours", "house", "how", "how's",
	"however", "human", "hundred", "husband",

	"i", "i'll", "i'm", "i've", "idea", "ideas", "if", "image",
	"in", "include", "income", "indeed", "inside", "instead",
	"into", "is", "island", "issue", "it", "it's", "its",
	"itself",

	"job", "just", "justice",

	"keep", "kept", "kind", "knew", "know", "known",

	"lack", "land", "large", "larger", "last", "late",
	"later", "latter", "law", "lay", "lead", "learned",
	"least", "leave", "led", "left", "length", "less", "let",
	"letter", "letters", "level", "life", "light", "like",
	"likely", "line", "lines", "list", "little", "live",
	"lived", "living", "local", "long", "longer", "look",
	"looked", "looking", "lost", "lot", "love", "low", "lower",

	"made", "main", "major", "make", "makes", "making", "man",
	"man's", "manner", "many", "march", "market", "mass",
	"matter", "may", "maybe", "me", "mean", "meaning", "means",
	"medical", "meet", "meeting", "member", "members", "men",
	"merely", "met", "method", "methods", "middle", "might",
	"miles", "million", "mind", "minutes", "miss", "modern",
	"moment", "money", "month", "months", "moral", "more",
	"morning", "most", "mother", "move", "moved", "moving",
	"mr", "mrs", "much", "music", "must", "my", "myself",

	"name", "nation", "nations", "natural", "nature", "near",
	"nearly", "need", "needed", "needs", "neither", "never",
	"new", "next", "night", "no", "nor", "normal", "north",
	"not", "note", "nothing", "now", "nuclear", "number",
	"numbers",

	"of", "off", "office", "often", "oh", "old", "on", "once",
	"one", "ones", "only", "open", "opened", "or", "order",
	"other", "others", "our", "out", "outside", "over", "own",

	"paid", "paper", "part", "parts", "party", "passed",
	"past", "pattern", "pay", "peace", "people", "per",
	"perhaps", "period", "person", "persons", "picture",
	"piece", "pierce", "place", "placed", "plan", "plane",
	"plans", "plant", "play", "point", "points", "police",
	"policy", "pool", "poor", "power", "present", "press",
	"private", "problem", "process", "provide",
	"public", "purpose", "put",

	"quality", "quite",

	"radio", "ran", "range", "rate", "rather", "reached",
	"read", "reading", "ready", "real", "really", "reason",
	"recent", "record", "red", "report", "respect", "rest",
	"result", "results", "return", "right", "river", "road",
	"room", "run", "running",

	"said", "sales", "same", "sat", "saw", "say", "saying",
	"says", "school", "schools", "science", "second",
	"section", "see", "seem", "seemed", "seems", "seen",
	"sense", "sent", "series", "serious", "served", "service",
	"set", "seven", "several", "shall", "she", "short", "shot",
	"should", "show", "showed", "shown", "side", "similar",
	"simple", "simply", "since", "single", "six", "size",
	"slowly", "small", "so", "social", "society", "some",
	"son", "soon", "sort", "sound", "south", "soviet", "space",
	"speak", "special", "spirit", "spring", "square", "staff",
	"stage", "stand", "start", "started", "state", "states",
	"stay", "step", "steps", "still", "stock", "stood", "stop",
	"stopped", "story", "street", "strong", "student", "study",
	"subject", "such", "summer", "sun", "support", "sure",
	"surface", "system", "systems",

	"table", "take", "taken", "taking", "talk", "tax",
	"tell", "ten", "terms", "test", "than", "that", "that's",
	"the", "their", "them", "then", "theory", "there",
	"there's", "these", "they", "thing", "things", "think",
	"third", "this", "those", "though", "thought", "three",
	"through", "thus", "time", "times", "to", "today", "told",
	"too", "took", "top", "total", "toward", "town", "trade",
	"trial", "tried", "trouble", "true", "truth", "try",
	"trying", "turn", "turned", "two", "type", "types",

	"under", "union", "united", "until", "up", "upon", "us",
	"use", "used", "using", "usually",

	"value", "values", "various", "very", "view", "visit",
	"voice", "volume",

	"waiting", "walked", "wall", "want", "wanted", "war",
	"was", "wasn't", "water", "way", "ways", "we", "week",
	"weeks", "well", "went", "were", "west", "western", "what",
	"what's", "when", "when's", "where", "where's", "whether",
	"which", "while", "white", "who", "who's", "whole", "whom",
	"whose", "why", "wide", "wife", "will", "window", "wish",
	"with", "within", "without", "woman", "women", "word",
	"words", "work", "worked", "working", "works", "world",
	"would", "writing", "written", "wrong", "wrote",

	"year", "years", "yes", "yet", "you", "you're", "young", "your"
};

/* convert string to lower case */

lowstr(char *s)
{
	while (*s = tolower(*s))
		s++;
}


/* See if okay to overwrite an existing file. */

exists(char *file)
{
	if (access(file, 0) == 0) {
		printf("'%s' already exists...okay to overwrite (y/N): ", file);
		fflush(stdout);
		if (tolower(getchar()) != 'y')
			exit(0);
		putchar('\n');
	}
}

/* Compress a word list */

compress(char *dname, char *wordfile)
{
	char word[BUFFSIZE];
	int t, n;
	char *p;

	memset(compword, 0, BUFFSIZE);
	memset(oldword, 0, BUFFSIZE);

	puts("Creating new dictionary file");

	if ((wordpath = fopen(wordfile, "r")) == NULL)
		terminate("Can't open wordlist file %s for input, error %d",
			  wordfile, errno);

	exists(dname);		/* see if okay to write over existing file */

	if ((dictpath = fopen(dname, "w")) == NULL)
		terminate("Can't create dictionary %s, error %d", dname, errno);

	fseek(dictpath, sizeof(Smod), 0);	/* past header */
/*      header._mh._mname=ftell(dictpath);      /* insert name in file */
/*      fwrite(modname, strlen(modname)+1, 1, dictpath); */

	/* this is where the dict data starts */

	align(dictpath);
	header._doffset = ftell(dictpath);

	filepos = 0;

	for (t = 0; t < KEYS1; t++) {	/* set all indexes to -1 */
		for (n = 0; n < KEYS2; indexes[t][n++] = -1) ;
	}

	while (1) {
		if (fgets(word, sizeof(word), wordpath) == NULL)
			break;
		if (p = index(word, '\n'))
			*p = '\0';
		add_word(word);
	}
	putnib(EWORD);
	putnib(EWORD);
	putnib(EWORD);

	header._dendoffset = ftell(dictpath);

	/* write the indexes */

	align(dictpath);
	header._xoffset = ftell(dictpath);
	fwrite(indexes, sizeof(indexes), 1, dictpath);

	/* write the quickwords */

	align(dictpath);
	header._qoffset = ftell(dictpath);
	header._qwidth = QUICKMAX;
	header._qcount = sizeof(quickwords) / sizeof(quickwords[0]);
	fwrite(quickwords, sizeof(quickwords), 1, dictpath);

/*      fwrite(word, 4, 1, dictpath);  /* add dummy for crc */

/*      header._mh._msync=MODSYNC; */
/*      header._mh._msysrev=1; */
/*      header._mh._msize=ftell(dictpath); */
/*      header._mh._mowner=0; */
/*      header._mh._maccess=0x111;  /* access =group,public,owner read */
/*      header._mh._mtylan=mktypelang(MT_DATA, ML_ANY); */
/*      header._mh._mattrev=mkattrevs(MA_REENT+MA_GHOST, 0); */
/*      header._mh._medit=1; */
	fseek(dictpath, 0, 0);
	fwrite(&header, sizeof(header), 1, dictpath);

	fclose(dictpath);

/*      puts("\nFixing module CRC"); */
/*      sprintf(word, "fixmod -u %s", dname); */
/*      system(word); */

/*      if(strcmp(dname, modname)!=0) */
/*              printf("Please remember to rename '%s' to '%s'\n", dname, modname); */
/*      puts("The new dictionary file should be moved to the VED directory"); */
}

/* add a word to the dictionary */

add_word(char *w)
{
	char *w1, *w2;
	int t;
	char i1, i2, c;

	if (*w == '\0')
		goto EXIT;	/* null entry, skip */
	lowstr(w);		/* set all words to lowercase */
	t = strcmp(w, compword);
	if (t == 0)
		goto EXIT;	/* duplicate word, skip */

	if (t < 0)
		terminate("Input word list not in order (%s)", w);

	if (*w != *compword) {
		putchar(*w);	/* progress letter */
		fflush(stdout);
	}
	if (w[0] > compword[0] || (w[1] > compword[1] && w[1] >= 'a')) {
		i1 = w[0];
		i2 = w[1];
		if (i1 >= 'a' && i1 <= 'z' && i2 < 'z') {
			i1 -= 'a';
			i2 = (i2 < 'a') ? 0 : i2 - 'a' + 1;
			indexes[i1][i2] = filepos;
		}
	}
	strcpy(compword, w);

	for (w1 = w, w2 = oldword, t = 0; *w2 && t < 15;) {
		if (*w1 != *w2)
			break;
		t++;
		w1++;
		w2++;
	}
	putnib(EWORD);		/* new word nibble */
	putnib(t);		/* size of new old sect to use */
	for (; *w1; w1++) {
		c = *w1;
		if (w2 = index(chars1, c))
			putnib(w2 - chars1);
		else if (w2 = index(chars2, c)) {
			putnib(ALTCHAR);
			putnib(w2 - chars2);
		} else
			terminate("Uncodeable character (%s) in file", w);
	}
	strcpy(oldword, w);

      EXIT:;
}


putnib(int n)
{
	static u_char c;
	static char phase = 0;

	if (phase = !phase)
		c = n << 4;
	else {
		c += n;
		putc(c, dictpath);
	}
	filepos++;
}

/* Uncompress a dictionary file */

uncompress(char *dname, char *wordfile)
{
	char *get_word(void);
	char *w;
	char flag = 0;

	puts("Extracting words from existing dictionary");

	if ((dictpath = fopen(dname, "r")) == NULL)
		terminate("Can't open dictionary %s for reading", dname, errno);

	fread(&header, sizeof(header), 1, dictpath);
	fseek(dictpath, header._doffset, 0);

	dictcount = header._dendoffset - header._doffset;

	exists(wordfile);	/* see if okay to overwrite if file exists */

	if ((wordpath = fopen(wordfile, "w")) == NULL)
		terminate("Can't open %s for output, error %d", wordfile, errno);

	while (1) {
		w = get_word();
		if (!w)
			break;
		if (!*w)
			continue;	/* skip null words */
		if (*w > flag) {
			flag = *w;
			putchar(flag);
			fflush(stdout);
		}
		fputs(w, wordpath);
		putc('\n', wordpath);
	}
}

char *
get_word(void)
{
	static char word[100];
	static char first = 0;

	char *w;
	int c;

	if (first == 0) {
		w = word;
		first++;
	} else {
		if ((c = getnib()) == -1)
			return NULL;
		w = word + c;
	}

	while (1) {
		if ((c = getnib()) == -1)
			return NULL;
		if (c == ALTCHAR)
			*w++ = chars2[getnib()];
		else if (c == EWORD) {
			*w = '\0';
			return word;
		} else
			*w++ = chars1[c];
	}
}

getnib(void)
{
	static u_char c;
	static u_char phase = 0;
	int t;

	if (dictcount == 0)
		return -1;

	if (phase = ~phase) {
		dictcount--;
		if ((t = getc(dictpath)) == -1)
			return -1;
		c = t;
		return (c >> 4);
	} else
		return (c & 0xf);
}

/* Univeral exit routine, displays message */

terminate(char *s, char *p1, char *p2, char *p3, char *p4, char *p5)
{
	putchar('\n');
	printf(s, p1, p2, p3, p4, p5);
	putchar('\n');
	exit(0);
}

/* Display usage message */

usage(void)
{
	static char *msg[] =
	{
		"Vspell_maint, (c) 1996",
		"Bob van der Poel Software",
		"Usage: vspell -option Dictfile Wordfile",
	     "Options: -c compress ascii word list to vspell dictionary",
		"         -u extract ascii list from existing dictionary",
		"Dictfile - existing (or new) vspell dictionary filename",
		"Wordfile - existing (or new) sorted word list filename",
		"NOTE: all arguments must be specified in order shown"
	};

	int t;

	for (t = 0; t < sizeof(msg) / sizeof(msg[0]); puts(msg[t++])) ;

	exit(0);
}

/* =============== M A I N ================== */

main(int argc, char **argv)
{
	if (argc != 4)
		usage();
	if (argv[1][0] != '-')
		usage();

	setbuf(stdin, NULL);	/* set stdin to unbuffered */

	switch (argv[1][1]) {
		case 'U':
		case 'u':
			uncompress(argv[2], argv[3]);
			break;

		case 'C':
		case 'c':
			compress(argv[2], argv[3]);
			break;

		default:
			usage();
	}
}

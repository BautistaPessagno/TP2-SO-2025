#include <sys.h>
#include <stdio.h>

static int isVowel(char c) {
	return c=='a'||c=='e'||c=='i'||c=='o'||c=='u'||
	       c=='A'||c=='E'||c=='I'||c=='O'||c=='U';
}

int cmd_mem(int argc, char **argv) {
	(void)argv;
	MMState st = {0,0,0};
	if (getMemoryState(&st) != 0) {
		perror("mem: unavailable\n");
		return 1;
	}
	printf("total=%d bytes, used=%d bytes, free=%d bytes\n", (int)st.total, (int)st.allocated, (int)st.available);
	return 0;
}

int cmd_ps(int argc, char **argv) {
	(void)argc; (void)argv;
	return printProcesses();
}

int cmd_loop(int argc, char **argv) {
	int periodMs = 1000;
	if (argc > 0) {
		sscanf(argv[0], "%d", &periodMs);
		if (periodMs <= 0) periodMs = 1000;
	}
	int pid = getpid();
	while (1) {
		printf("[loop pid=%d] running\n", pid);
		sleep((uint32_t)periodMs);
		yield();
	}
	return 0;
}

int cmd_kill(int argc, char **argv) {
	if (argc < 1) {
		perror("kill: missing pid\n");
		return 1;
	}
	int pid = -1;
	sscanf(argv[0], "%d", &pid);
	int32_t r = killProcess((uint16_t)pid);
	if (r < 0) {
		perror("kill: failed\n");
		return 1;
	}
	printf("killed %d\n", pid);
	return 0;
}

int cmd_nice(int argc, char **argv) {
	if (argc < 2) {
		perror("nice: usage nice [pid] [priority]\n");
		return 1;
	}
	int pid = -1, prio = -1;
	sscanf(argv[0], "%d", &pid);
	sscanf(argv[1], "%d", &prio);
	if (prio < 0) prio = 0;
	if (prio > 4) prio = 4;
	int32_t r = nice((uint16_t)pid, (uint8_t)prio);
	if (r < 0) {
		perror("nice: failed\n");
		return 1;
	}
	printf("pid %d priority -> %d\n", pid, prio);
	return 0;
}

int cmd_block(int argc, char **argv) {
	if (argc < 1) {
		perror("block: missing pid\n");
		return 1;
	}
	int pid = -1;
	sscanf(argv[0], "%d", &pid);
	int32_t r = block((uint16_t)pid);
	if (r < 0) {
		r = unblock((uint16_t)pid);
		if (r < 0) {
			perror("block: failed\n");
			return 1;
		}
		printf("unblocked %d\n", pid);
		return 0;
	}
	printf("blocked %d\n", pid);
	return 0;
}

int cmd_cat(int argc, char **argv) {
	(void)argc; (void)argv;
	int c;
	while ((c = getchar()) != '\n' && c != -1) {
		putchar((char)c);
	}
	putchar('\n');
	return 0;
}

int cmd_wc(int argc, char **argv) {
	(void)argc; (void)argv;
	int c;
	int bytes = 0, words = 0, lines = 0;
	int in_word = 0;
	while ((c = getchar()) != '\n' && c != -1) {
		bytes++;
		if (c == '\n') lines++;
		if (c == ' ' || c == '\t' || c == '\n') {
			if (in_word) {
				words++;
				in_word = 0;
			}
		} else {
			in_word = 1;
		}
	}
	// Account last word if last char wasn't delimiter
	if (in_word) words++;
	printf("lines=%d words=%d bytes=%d\n", lines, words, bytes);
	return 0;
}

int cmd_filter(int argc, char **argv) {
	(void)argc; (void)argv;
	int c;
	while ((c = getchar()) != '\n' && c != -1) {
		if (!isVowel((char)c)) putchar((char)c);
	}
	putchar('\n');
	return 0;
}


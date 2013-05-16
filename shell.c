/*
 * ��� shell �� UNIX v6 �� sh �� POSIX �����µ�����ʵ�֡�
 * UNIX v6 ���� BSD ���֤������������������� sh ��ԭ������ Ken Thompson��
 * ������ shell ���﷨����ϸ���޸ģ��Դ���������д��ע�͡�
 * ���������������ĵ������κε���������һ��Ȩ�������е��κ����κ�����
 *
 * ����޶�: 2006-07-13   ������ʿ(http://mhss.cublog.cn)
 * ���Ĺ�����Ҫ��: K&R C -> ANSI C��unix v6/v7 -> POSIX��ȥ���˽��̼��ʺ� ^��
 * ������ $?��umask �� exec��ȥ���� goto ��䣬����������ע�͡�
 * �ϴ��޶�: 2004-09-06
 */

/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met: 
 *
 *     Redistributions of source code and documentation must retain the above
 *     copyright notice, this list of conditions and the following disclaimer.
 *     Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution. 
 *
 *     All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement: 
 *
 *     This product includes software developed or owned by Caldera International, Inc. 
 *
 *     Neither the name of Caldera International, Inc. nor the names of other
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission. 
 * 
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA INTERNATIONAL, INC. 
 * AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE FOR 
 * ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE. 
 */

#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define	QUOTE 0x80 /* ���ñ�־λ���������ַ���Ϊ 7 λ ASCII */

/* �﷨���ڵ��ֶζ��� */
#define	DTYP 0 /* type: �ڵ����� */
#define	DLEF 1 /* left: ���ӽڵ�������ض����ļ������� */
#define	DRIT 2 /* right: ���ӽڵ������ض����ļ������� */
#define	DFLG 3 /* flag: ��־λ���﷨���ڵ����� */
#define	DSPR 4 /* space: �ڼ������ʱ��ԭ�汾�ǿ�λ�����ǲ���������
                * parentheses: �ڸ�������ʱָ�����﷨�� */
#define	DCOM 5 /* command: �������б� */

/* ���Ͷ��壬���� DTYP */
#define	TCOM 1 /* command: ������ */
#define	TPAR 2 /* parentheses: �������� */
#define	TFIL 3 /* filter: ������ */
#define	TLST 4 /* list: �����б� */

/* ��־���壬���� DFLG */
#define	FAND 0x01 /* and: & ��ִ̨�� */
#define	FCAT 0x02 /* catenate: >> ��ӷ�ʽ������ض��� */
#define	FPIN 0x04 /* pipe in: ����ӹܵ�������� */
#define	FPOU 0x08 /* pipe out: ������ܵ�������� */
#define	FPAR 0x10 /* parentheses: �������������һ������ */
#define	FINT 0x20 /* interrupt: ��̨���̺����ж��ź� */
#define	FPRS 0x40 /* print string: ��ӡ��̨���� pid */

char	*dolp = NULL; /* dollar p: ָ�������в�����ָ�� */
char	**dolv; /* dollar v: �����в����б� */
int 	dolc; /* dollar c: �����в������� */
char	pidp[11]; /* ���� sh ������� pid ���ַ��� */
char	*promp; /* ��ʾ�� */
int 	peekc = 0; /* Ԥ���ַ� */
int 	gflg = 0; /* ȫ�ֱ�־��������;������������������ͨ��� */
int 	error; /* �﷨�������ִ����־ */
int 	setintr = 0; /* �����ж��źź��Եı�־ */
char	*arginp = NULL; /* ָ�����Ҫִ�е�����Ĳ�����ִ��֮���˳� */
int 	onelflg = 0; /* one line flag: ��ȡһ������ִ�в��˳���־ */
uid_t	uid; /* sh ���̵���ʵ uid */
jmp_buf	jmpbuf; /* �纯����ת�������浱ǰ״̬�Ļ����� */
int 	exitstat = 0; /* ִ���������ֹ״̬ */
char	exitp[4]; /* ������ֹ״̬���ַ��� */

char	*linep, *elinep; /* �����л�������ָ�� */
char	**argp, **eargp; /* �������б�洢�ռ��ָ�� */
int 	*treep, *treeend; /* �����﷨���ڵ�洢�ռ��ָ�� */

#define	LINSIZ _POSIX_ARG_MAX
#define	ARGSIZ LINSIZ/20 /* �ٶ�ƽ��ÿ������ 20 ���ַ� */
#define	TRESIZ ARGSIZ*2
char	line[LINSIZ]; /* �����л����� */
char	*args[ARGSIZ]; /* ���б� */
int 	trebuf[TRESIZ]; /* �洢�﷨���ڵ� */

/* �����Ϣ */
#define NSIGMSG 18
char *mesg[NSIGMSG] = {
	NULL,
	"Hangup", /* SIGHUP 1 */
	NULL, /* SIGINT 2 */
	"Quit", /* SIGQUIT 3 */
	"Illegal instruction", /* SIGILL 4 */
	"Trace trap", /* SIGTRAP 5 */
	"Abort", /* SIGABRT 6 */
	"Signal 7", /* SIGEMT 7 */
	"Floating point exception", /* SIGFPE 8 */
	"Killed", /* SIGKILL 9 */
	"Signal 10", /* SIGBUS 10 */	
	"Segmentation violation", /* SIGSEGV 11 */
	"Bad argument to system call", /* SIGSYS 12 */
	NULL, /* SIGPIPE 13 */
	"Alarm clock", /* SIGALRM 14 */
	"Software termination signal from kill", /* SIGTERM 15 */
	"Signal 16", /* 16 */
	"Child process terminated or stopped",/* SIGSTOP 17 */
};

void lexscan(void);
void word(void);
int  nextc(void);
int  readc(void);
int  *parse(void);
int  *cmdlist(char **p1, char **p2);
int  *pipeline(char **p1, char **p2);
int  *command(char **p1, char **p2);
int  *tree(int n);
void execute(int *t, int *pf1, int *pf2);
int  redirect(int *t);
int  builtin(int *t);
int  execcmd(int *t);
int  texec(char *f, char **t);
void scan(char **t, int (*f)());
int  tglob(int c);
int  trim(int c);
void err(char *s);
void prs(char *as);
void prc(int c);
void prn(int n);
void sprn(int n, char *s);
int  any(int c, char *as);
int  equal(char *as1, char *as2);
int  pwait(pid_t i);

extern int glob(int argc, char *argv[]);

int main(int argc, char **argv)
{
	register int f;	/* �ļ������� */
	int *t; /* �﷨�� */

	/* �رմ򿪵��ļ� */
	for(f=STDERR_FILENO; f<sysconf(_SC_OPEN_MAX); f++)
		close(f);
	/* ���Ʊ�׼�������׼������� */
	if((f=dup(STDOUT_FILENO)) != STDERR_FILENO)
		close(f);

	/* ��ý��̱�ʶ(32λ����)����ת�����ַ�������ʱ���� dolc ���� */
	dolc = (int)getpid();
	sprn(dolc, pidp);
	/* �жϵ�ǰʱ���Ƿ���û���������ȷ����ʾ�� */
	if((uid = getuid()) == 0) /* ���û� */
		promp = "# ";
	else
		promp = "% ";
	setuid(uid);
	setgid(getgid());

	if(argc > 1) {	/* �в�����ѡ�� */
		promp = 0;	/* ����Ϊ�ǽ���ģʽ */
		if (*argv[1]=='-') { /* ��ѡ�� */
			**argv = '-';  /* �Ѳ��� 0 �ĵ�һ���ַ�����Ϊ - */
			if (argv[1][1]=='c' && argc>2) /* -c ����һ��������Ϊ����ִ�� */
				arginp = argv[2];
			else if (argv[1][1]=='t') /* -t: �ӱ�׼�������һ��ִ�в��˳� */
				onelflg = 2;
		} else { /* �������ļ� */
			/* �ڱ�׼�����ϴ򿪰���Ҫִ�е�������ļ� */
			close(STDIN_FILENO);
			f = open(argv[1], O_RDONLY);
			if(f < 0) { /* �򲻿�ָ���ļ� */
				prs(argv[1]);
				err(": cannot open");
			}
		}
	}
	if(**argv == '-') { /* ��ѡ�� */
		setintr++;
		/* �����ж��źŴ������Ϊ�����ź� */
		signal(SIGQUIT, SIG_IGN);
		signal(SIGINT, SIG_IGN);
	}
	dolv = argv+1; /* �����б�ָ������ */
	dolc = argc-1; /* ������Ŀ���� */

	/* ��ѭ��: ɨ�������У�������ִ���﷨�� */
	for(;;) {
		error = 0;
		gflg = 0;
		if(promp != 0)	/* ����ģʽ���� */
			prs(promp);	
		lexscan();
		if(gflg != 0) {/* �����������ַ���� */
			err("Command line overflow");
			continue;
		}
		t = parse();
		if(error != 0) {/* �﷨�������ִ��� */
			err("syntax error");
			continue;
		}
		execute(t,NULL,NULL);
	}
	return 0;
}

/*
 * �ʷ�ɨ��
 */
void lexscan(void)
{
	register char *cp;
	register int c;
	/* ��ʼ���л����������б�ռ�����ָ�� */
	argp = args+1; /* �ճ�һ��λ�� */
	eargp = args+ARGSIZ-5;
	linep = line;
	elinep = line+LINSIZ-5;

	/* ���˵�ע���� */
	do c = nextc();
	while (c == ' ' || c == '\t');
	if (c == '#')
		while ((c = nextc()) != '\n');
	peekc = (char) c; /* �ͻ����Ļ��з� */
	
	/* ��������ɨ�赽���б��� */
	do {
		cp = linep; /* cp ָ��ǰҪ����������л������е�λ�� */
		word(); /* ����һ���ֵ��л������� */
	} while(*cp != '\n'); /* ѭ��ֱ���������з� */
}

/*
 * �������ж���һ���ֵ��л������У�������һ�����б�Ԫ��
 * ������ lexscan() 
 * ���� nextc() readc()
 */
void word(void)
{
	register int c, c1;
	
	/* ���б�ĵ�ǰԪ��ָ��ָ��ǰҪ����������л������е�λ�� */
	*argp++ = linep;

	/* ������ǰ�հ� */
	do c = nextc();
	while (c == ' ' || c == '\t');
	/* ���� shell ��Ԫ�ַ��ͻ��з� */
	if(any(c, ";&<>()|\n")) {	 
		*linep++ = (char) c; /* �����Ԫ�ַ����з�д���л������� */
		*linep++ = '\0'; /* �ս�����ַ��� */
		return;	
	}
	/* ������ͨ�ַ� */
	peekc = c; /* �ͻ������ͨ�ַ� */
	for(;;) {
		c = nextc();
		if(any(c, " \t;&<>()|\n")) { /* �����հס�Ԫ�ַ����з� */
			peekc = (char) c; /* �ͻ�����ַ� */
			*linep++ = '\0'; /* �ս�����ַ��� */
			return;
		}
		if(c == '\'' || c == '"') {/* ���������Ż�˫���� */
			c1 = c; /* �Թ���������Ż�˫���� */
			while((c=readc()) != c1) {
				if(c == '\n') {
					error++;	/* ����û���ڱ������ */
					peekc = (char) c; /* �ͻ�����ַ� */
					return;
				}
				/* �����Ű�Χ���ַ��������ñ�־λ����д���л������� */
				*linep++ = (char) c|QUOTE;
			}
			continue; /* �Թ�ƥ��ĵ����Ż�˫���� */ 
		}
		*linep++ = (char) c; /* �������ͨ�ַ�д���л����� */
	}
}

/*
 * �������ж���һ���ַ������б����滻
 * ������ word()
 * ���� readc()
 */
int nextc(void)
{
	int c;

	if(peekc) { /* �Ѿ�Ԥ����һ���ַ� */
		c = peekc;
		peekc = 0;
		return c;
	}
	if(argp > eargp) { /* ���б�ռ���� */
		argp -= 10;
		while((c=nextc()) != '\n'); /* ���Զ�����������ַ� */
		argp += 10;
		err("Too many args");
		gflg++;
		return c;
	}
	if(linep > elinep) {  /* �л������ռ���� */
		linep -= 10;
		while((c=nextc()) != '\n'); /* ���Զ�����������ַ� */
		linep += 10;
		err("Too many characters");
		gflg++;
		return c;
	}
	if(dolp == NULL) { /* ��ǰδ���� $ */
		c = readc();
		if(c == '\\') { /* ת��� */
			c = readc();
			if(c == '\n') /* �н��� */
				return ' ';
			return(c|QUOTE); /* ��������ַ� */
		}
		if(c == '$') { /* �����滻 */
			c = readc();
			if(c>='0' && c<='9') { /* λ�ò��� */
				if(c-'0' < dolc)
					dolp = dolv[c-'0'];
			}
			if(c == '$') { /* ���̱�ʶ */
				dolp = pidp;
			}
			if(c == '?') { /* �˳�״̬ */
				sprn(exitstat, exitp);
				dolp = exitp;
			}
		}
	}
	if (dolp != NULL) { /* ��ǰ���� $ */
		c = *dolp++;
		if(c != '\0')
			return c;
		dolp = NULL;
	}
	return c&~QUOTE; /* �������λ */
}

/*
 * �������ж�ȡһ���ַ�
 * ������ word() nextc()
 */
int readc(void)
{
	char cc;
	int c;

	if (arginp) { /* �� -c ѡ�onelflg == 0 */
		/* �� arginp �ж�ȡ�ַ� */
		if ((c = (int)*arginp++) == 0) { /* û��Ҫ��Ϊ����Ĳ��� */
			arginp = NULL; 
			onelflg++; /* �����´�ִ�б�������ʱ���˳� */
			c = '\n';
		}
		return c;
	}
	if (onelflg==1)
		exit(0);
	if(read(STDIN_FILENO, &cc, 1) != 1) /* ��һ���ַ� */
		exit(0);
	/* �� -t ѡ�onelflg == 2���ӱ�׼��������˻��з� */
	if (cc=='\n' && onelflg)
		onelflg--; /* �����´�ִ�б�������ʱ���˳� */
	return (int)cc;
}

/*
 * �﷨����
 */
int * parse(void)
{
	/* ��ʼ���﷨���ڵ�ռ�����ָ�� */
	treep = trebuf;
	treeend = &trebuf[TRESIZ];
	char **p = args+1; /* ��Ӧ��������λ */

	if (setjmp(jmpbuf) != 0)
	    /* û�пռ�����﷨���ڵ���� */
		return NULL;

	/* ����ǰ���Ļ��з� */
	while(p != argp && (int)**p == '\n')
		p++;
	/* args ָ�������б�ĵ�һ��Ԫ�أ�
	 * argp ָ�������б�����һ��Ԫ�غ����һ��Ԫ�� */
	return  cmdlist(p, argp);
}

/* 
 *  �����б�
 *	cmdlist:
 *		empty
 *		pipeline
 *		pipeline & cmdlist
 *		pipeline ; cmdlist
 */
int * cmdlist(char **p1, char **p2)
{
	register char **p;
	int *t, *t1;
	int l;

	if (p1 == p2)
		return NULL; /* �������б� */

	l = 0; /* Ƕ�ײ��� */
	for(p=p1; p!=p2; p++)
		switch(**p) {

		case '(':
			l++;
			break;

		case ')':
			l--;
			if(l < 0)
				error++;
			break;

		/* �ҵ�һ���б�ָ��� */
		case '&':
		case ';':
		case '\n':
			if(l == 0) { /* �������б�ָ�������Բ���Ű�Χ�� */
				l = **p;
				t = tree(4);
				t[DTYP] = TLST; /* �����������б� */
				t[DLEF] = (int)pipeline(p1, p); 
				t[DFLG] = 0;  /* �����б�ڵ㲻�����κα�־λ */
				if(l == '&') { /* ��Ҫ��̨���� */
					t1 = (int *)t[DLEF];
					/* �������ӽڵ��־ */
					t1[DFLG] |= FAND|FPRS|FINT;	
					/* 
					 * ��̨���е���Щ��־λֻ���õ��ڹܵ��߽ڵ��ϣ�
					 * ������ִ�е�ʱ����Ҫͨ���̳����´�����������ڵ��ϡ�
					 */
				}
				/* �϶�������б�ָ���Ϊ�﷨���� */
				if (any((int)*(p+1),";&")) {
					error++;
					return NULL;
				}
				t[DRIT] = (int)cmdlist(p+1, p2); 
				return t;
			}
		}
	/* û���ҵ��б�ָ�������������б������Բ������ */
	if(l == 0)
		return pipeline(p1, p2);
	error++;
	return NULL;
}

/*
 *  �ܵ��� 
 *	pipeline:
 *		command
 *		command | pipeline
 */
int * pipeline(char **p1, char **p2)
{
	register char **p;
	int l, *t;

	l = 0; /* Ƕ�ײ��� */
	for(p=p1; p!=p2; p++)
		switch(**p) {

		case '(':
			l++;
			break;

		case ')':
			l--;
			break;

		/* �ҵ�һ���ܵ����� */
		case '|':
			if(l == 0) { /* ����ܵ����Ų������Ű�Ϊ��Χ�� */
				t = tree(4);
				t[DTYP] = TFIL;	/* �����ǹܵ��� */
				t[DLEF] = (int)command(p1, p);
				t[DRIT] = (int)pipeline(p+1, p2);
				t[DFLG] = 0; /* ��־λ���ϼ��� cmdlist ���� */
				return t;
			}
		}
	/* û���ҵ��ܵ����ţ��ǹܵ���ĩ�˻������ */
	return command(p1, p2);
}

/*
 *  ����
 *	command:
 *		( cmdlist ) [ < in  ] [ > out ]
 *		word word* [ < in ] [ > out ]
 */
int * command(char **p1, char **p2)
{
	register char **p;
	char **lp = NULL, **rp = NULL; 
	int *t;
	int n = 0, l = 0, i = 0, o = 0, c, flg = 0;

	if(**p2 == ')')	/* �����������Բ���Ű�Χ�е������б�����һ������ */
		flg |= FPAR; /* �������Ҫ�� subshell �Ľ�����ִ�� */

	for(p=p1; p!=p2; p++)
		switch(c = **p) {

		case '(':
			if(l == 0) {
				if(lp != NULL)
					error++;
				lp = p+1; /* �����Բ�������б�ĵ�һ���� */
			}
			l++;
			break;

		case ')':
			l--;
			if(l == 0)
				rp = p; /* �����Բ�����ڵ����һ���ֺ����')' */
			break;

		case '>':
			p++;
			if(p!=p2 && **p=='>') /*  >> �ض��� */
				flg |= FCAT;
			else
				p--;
		case '<':
			if(l == 0) {
				p++; /* �ض�����ļ������� */
				if(p == p2) { /* �ض������֮��û���ַ��� */
					error++;
					p--;
				}
				if(any(**p, "<>("))  /* �ض����ļ�����ͷ���ַ����Ϸ� */
					error++;
				if(c == '<') {
					if(i != 0)
						error++;
					i = (int)*p; /* ���������ض����ļ���ȫ·���� */
				} else { /* > �� >> */
					if(o != 0)
						error++;
					o = (int)*p; /* ��������ض����ļ���ȫ·���� */
				}
				break;
			}

		default: 
			if(l == 0) /* �����ͨ���ֲ���Բ���Ű�Χ�� */
				p1[n++] = *p;
			/* �γɲ����б�ǰ�Ƽ������ض�����ź���Ӧ���ļ��������� */
		}

	if(lp != 0) { /* ��Բ���� */
		if(n != 0)  /* ���в���Բ�����е���ͨ���� */
			error++;
		t = tree(5); /* �� DSPR �ֶ� */
		t[DTYP] = TPAR; /* �����Ǹ������� */
		t[DSPR] = (int)cmdlist(lp, rp);
	} else { /* ������ */
		if(n == 0)	/* û���������� */
			error++; 
		t = tree(6);
		/* ���﷨���ڵ����� DSPR��DCOM �ֶ� */
		t[DTYP] = TCOM; /* �����Ǽ����� */
		t[DSPR] = n; /* ���б�Ԫ����Ŀ */
		t[DCOM] = (int)p1; /* ���б� */
	}
	t[DFLG] = flg; /* ����������õ� FPAR �� FCAT ��־ */
	t[DLEF] = i; /* �����ض����ļ���ȫ·���� */
	t[DRIT] = o; /* ����ض����ļ���ȫ·���� */
	return t;
}

/*
 * ����ָ����С�������
 * ������ cmdlist() pipeline() command()
 */
int * tree(int n)
{
	int *t;

	t = treep;
	treep += n;
	if (treep>treeend) { /* �﷨���ڵ�ռ䲻�� */
		prs("Command line overflow\n");
		error++;
		longjmp(jmpbuf,1);
	}
	return t;
}

/* 
 * �ܵ����﷨����ִ�У�pf1 ����ܵ���pf2 �����ܵ�
 * a | b | c �ܵ��ߵ��﷨����
 *     TFIL1
 *     /   \
 *  TCOM1 TFIL2 
 *    |   /   \
 *    a TCOM2 TCOM3
 *        |     |
 *        b     c
 * ִ�����У�
 * execute(TFIL1, NULL, NULL);
 * execute(TOCM1, NULL, pv1);	TOCM1.TFLG: FPOU;	�����̴��� pv1
 * execute(TFIL2, pv1, NULL);	TFIL2.TFLG: FPIN;
 * execute(TCOM2, pv1, pv2);	TCOM2.TFLG: FPIN, FPOU;	�����̹ر��� pv1, ���� pv2
 * execute(TCOM3, pv2, NULL);	TCOM3.TFLG: FPIN;	�����̹ر��� pv2
 */
void execute(int *t, int *pf1, int *pf2)
{
	int i, f, pv[2];
	register int *t1;

	if(t == 0)
		return;
	switch(t[DTYP]) {
	case TLST:	/* �����б����� */
		f = t[DFLG];	
		if((t1 = (int *)t[DLEF]) != NULL)
			/* ���ӽڵ��´� FINT ��־�ĵ�ǰ״̬ */
			t1[DFLG] |= f&FINT;	
		execute(t1,NULL,NULL);
		if((t1 = (int *)t[DRIT]) != NULL)
			t1[DFLG] |= f&FINT;
		execute(t1,NULL,NULL);
		/*
		 * �����б�λ�ڸ��������е��е�ʱ��TLST �ڵ�
		 * ֻ���ϲ� TPAR �ڵ�̳� FINT ��־�� 
		 */
		return;

	case TFIL:	/* �ܵ������� */
		f = t[DFLG];
		pipe(pv); /* �����ܵ� */
		t1 = (int *)t[DLEF];
		/* �����ӽڵ��´� FPIN FINT FPRS ��־�ĵ�ǰ״̬��
		 * ���������� FPOU ��־  */
		t1[DFLG] |= FPOU | (f&(FPIN|FINT|FPRS));
		execute(t1, pf1, pv); /* ������������½��Ĺܵ� */
		t1 = (int *)t[DRIT];
		/* �����ӽڵ��´� FPOU FINT FAND FPRS ��־�ĵ�ǰ״̬��
		 * ���������� FPIN ��־  */
		t1[DFLG] |= FPIN | (f&(FPOU|FINT|FAND|FPRS));
		execute(t1, pv, pf2); /* �������������½��Ĺܵ� */
		/*
		 * ֻ�йܵ���ĩ�˵������ܴ��ϲ�ڵ�̳е� FAND ��־
		 */
		return;

	case TCOM: /* ������������� */
		/* ɸѡ���������� */
		if (builtin(t))
			return;

	case TPAR: /* ����������������� */
		f = t[DFLG];
		i = 0;
		/* 
		 * Ϊ�����������������͸�����������һ������֮�����������
		 * �����ӽ��̣������������һ������ʹ��Ϊ�����������彨�����ӽ���
		 */
		if((f&FPAR) == 0) 
			i = fork(); 
		if(i == -1) { /* ���̸���ʧ�� */
			err("try again");
			return;
		}
		/*
		 * ������ִ�в���
		 */
		if(i != 0) { 
			if((f&FPIN) != 0) { /* �ӽ���������ܵ� */
				/* �ڶ��ܵ����ӽ���û�н���֮ǰ�������̱��ִ�����ܵ�
				 * �����ܵ����ӽ����Ѿ�����֮�󣬸����̹رչܵ�����д��� */
				close(pf1[0]); 
				close(pf1[1]);
			}
			if((f&FPRS) != 0) { /* ��Ҫ��ӡ�ӽ��� pid */
				prn(i);
				prs("\n");
			}
			if((f&FAND) != 0) { /* ��̨�����Ҫ�ȴ��ӽ��� */
				exitstat = 0;
				return;
			}
			if((f&FPOU) == 0) /* �ӽ�����ǰ̨�ļ������ܵ���ĩ�˵����� */
				exitstat = pwait(i); /*  �ȴ��ӽ�����ֹ */
			return;
		}
		/*
		 * �ӽ���ִ�в���
		 */
		if (redirect(t)) /* �ض����׼������� */
			exit(1);
		if((f&FPIN) != 0) { /* ������ܵ� */
			close(STDIN_FILENO);
			/* ����ܵ������� */
			dup(pf1[0]);
			close(pf1[0]);
			/* �رչܵ�д��� */
			close(pf1[1]);
		}
		if((f&FPOU) != 0) {	/* �������ܵ� */
			/* ����ܵ�д��� */
			close(STDOUT_FILENO);
			dup(pf2[1]);
			close(pf2[0]);
			/* �رչܵ������� */
			close(pf2[1]);
		}
		/* ��̨����û�����ض������׼���� */
		if((f&FINT)!=0 && t[DLEF]==0 && (f&FPIN)==0) {
			close(STDIN_FILENO);
			open("/dev/null", O_RDONLY);
		}
		/* ��δ�����ж��źź��Ա�־��ǰ̨���
		 * ���ڷǽ���ģʽ���Ѿ������ж��źŴ������Ϊ�����ź� */
		if((f&FINT) == 0 && setintr) {
			/* �ָ��ж��źŴ������Ϊȱʡ */
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
		}
		/* 
		 * ������������ shell ��ִ�У��� shell �ڵ��е� FAND, FPRS ��־λ������
		 * �� shell ����Ľ��̣��� shell �еĽڵ㲻�̳� FAND, FPRS ��־λ��
		 */
		if(t[DTYP] == TPAR) { 
			if((t1 = (int *)t[DSPR]) != NULL)
				/* ���ӽڵ��´� FINT ��־�ĵ�ǰ״̬ */
				t1[DFLG] |= f&FINT;
			execute(t1,NULL,NULL);
			exit(1);
		}
		i=execcmd(t); /* ִ������ */
		exit(i);
	}
}

/*
 * �ض����׼�������
 */
int redirect(int *t)
{
	int i;
	int f = t[DFLG];
	
	if(t[DLEF] != 0) { /* �������ض��� */
		/* �ض����׼���� */
		close(STDIN_FILENO);
		i = open((char *)t[DLEF], O_RDONLY);
		if(i < 0) {
			prs((char *)t[DLEF]);
			err(": cannot open");
			return 1;
		}
	}
	if(t[DRIT] != 0) { /* ������ض��� */
		if((f&FCAT) != 0) { /* >> �ض��� */
			i = open((char *)t[DRIT], O_WRONLY);
			if(i >= 0)
				lseek(i, 0, SEEK_END);
		} else {
			i = creat((char *)t[DRIT], 0666);
			if(i < 0) {
				prs((char *)t[DRIT]);
				err(": cannot create");
				return 1;
			}
		}
		/* �ض����׼��� */
		close(STDOUT_FILENO);
		dup(i);
		close(i);
	}
	return 0;
}

/*
 * ���������
 */
int builtin(int *t)
{
	register char *cp1, *cp2;
	int c, i = 0;
	int ac = t[DSPR];
	char **av = (char**)(t[DCOM]);

	cp1 = av[0];
	if(equal(cp1, "cd") || equal(cp1, "chdir")) {
		if(ac == 2) { /* ��һ������ */
			if(chdir(av[1]) < 0)
				err("chdir: bad directory");
		} else
			err("chdir: arg count");
		return 1;
	}
	if(equal(cp1, "shift")) {
		if(dolc < 1) {
			prs("shift: no args\n");
			return 1;
		}
		dolv[1] = dolv[0]; /* �����ļ�����������˵�һ��λ�ò��� */
		dolv++;
		dolc--;
		return 1;
	}
	if(equal(cp1, "login")) {
		if(promp != 0) {
			av[ac] = NULL;
			execv("/bin/login", av);
		}
		prs("login: cannot execute\n");
		return 1;
	}
	if(equal(cp1, "newgrp")) {
		if(promp != 0) {
			av[ac] = NULL;
			execv("/bin/newgrp", av);
		}
		prs("newgrp: cannot execute\n");
		return 1;
	}
	if(equal(cp1, "wait")) {
		pwait(-1);	/* �ȴ������ӽ��� */
		return 1;
	}
	if(equal(cp1, ":"))
		return 1;
	if(equal(cp1, "exit")) {
		if (ac == 2) { /* ��һ������ */
			cp2 = av[1];
			while ((c = *cp2++) >= '0' && c <= '9')
				i = (i*10)+c-'0';
		}
		else if (ac > 2) {
			err("exit: arg count");
			return 1;
		}
		if(promp == 0) 
			lseek(STDIN_FILENO, 0, SEEK_END);
		exit(i);
	}
	if(equal(cp1,"exec")) {
		if (redirect(t)) {	/* �ض����׼������� */
			exitstat = 1;
			return 1;
		}
		if (ac > 1) { /* �в��� */
			t[DSPR] = ac-1;
			t[DCOM] = (int)(av+1);
			exitstat=execcmd(t);
			return 1;
		}
		exitstat = 0;
		return 1;
	}
	if(equal(cp1, "umask"))	{
		if (ac == 2) { /* ��һ������ */
			cp2 = av[1];
			while ((c = *cp2++) >= '0' && c <= '7')
				i = (i<<3)+c-'0';
			umask(i);
		}
		else if (ac > 2) 
			err("umask: arg count");
		else {
			umask(i = umask(0));
			prc('0');
			for (c = 6; c >= 0; c-=3)
				prc(((i>>c)&07)+'0');
			prc('\n');
		}
		return 1;
	}
	return 0;
}

/* 
 * ִ���﷨���ڵ��е�����, �����ļ���չ��
 * ������ execute() builtin()
 * ���� glob() texec()
 */
int execcmd(int *t)
{
	int ac = t[DSPR];	
	char **av = (char**)(t[DCOM]);

	av[ac] = NULL; /* �ս����б� */
	gflg = 0;
	scan(av, tglob);	/* ���Բ����Ƿ���ͨ��� */
	if(gflg) {
		av[-1]="/etc/glob";
		return glob(ac+1, av-1);
	}
	scan(av, trim); /* ������ñ�־λ */
	return texec(av[0], av);
}

/*
 * exec �ͽ��д�����Ϣ��ӡ
 * ������ execcmd() glob()
 */
int texec(char* f, char **t)
{
	extern int errno;

	/* ���ڲ����ý��̵Ļ������ɷ����ʹ�� execvp() ������ execve() */
	execvp(f, t);
	if (errno==EACCES) { /* û�з���Ȩ�� */
		prs(t[0]);
		err(": permission denied");
	}
	if (errno==ENOEXEC) { /* ���Ƕ����ƿ�ִ���ļ� */
		prs("No shell!\n");
	}
	if (errno==ENOMEM) { /* ���ܷ����ڴ� */
		prs(t[0]);
		err(": too large");
	}
	if (errno==ENOENT) {
		prs(t[0]);
		err(": not found");
		return 127; /* ָʾû���ҵ��ļ� */
	}
	return 126;
}

/*
 * ɨ�����б�
 */
void scan(char **t, int (*f)())
{
	register char *p, c;

	while((p = *t++) != NULL)
		while((c = *p) != 0)
			*p++ = (*f)(c);
}

/*
 * ����Ƿ����ͨ���
 */
int tglob(int c)
{
	if(any(c, "[?*"))
		gflg = 1;
	return c;
}

/*
 * ������ñ�־λ
 */
int trim(int c)
{
	return c&~QUOTE;
}

/*
 * ��ӡ����������ڷǽ���ģʽ���˳�
 */
void err(char *s)
{
	prs(s);
	prs("\n");
	if(promp == 0) {
		lseek(STDIN_FILENO, 0, SEEK_END);
		exit(1);
	}
}

/*
 * дһ���ַ���
 */
void prs(char *as)
{
	register char *s;

	s = as;
	while(*s)
		prc(*s++);
}

/*
 * дһ���ַ�
 */
void prc(int c)
{
	write(STDERR_FILENO, &c, 1);
}

/*
 * дһ����
 */
void prn(int n)
{
	register int a;

	if((a=n/10) != 0)
		prn(a);
	prc((n%10)+'0');
}

/*
 * дһ�������ַ���
 */
void sprn(int n, char *s)
{
	int i,j;
	for (i=1000000000; n<i && i>1; i/=10); /* 32 λ�ֳ� */
	for (j=0; i>0; j++,n%=i,i/=10)
		s[j] =(char)(n/i + '0');
	s[j] = '\0';
}

/*
 * �ж�һ���ַ��Ƿ���һ���ַ����У��ַ�'\0'�������κ��ַ�����
 */
int any(int c, char *as)
{
	register char *s;

	s = as;
	while(*s)
		if(*s++ == c)
			return 1;
	return 0;
}

/*
 * �ж������ַ����Ƿ����
 */
int equal(char *as1, char *as2)
{
	register char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while(*s1++ == *s2)
		if(*s2++ == '\0')
			return 1;
	return 0;
}

/*
 * i>0 �ȴ�ָ�� pid �Ľ�����ֹ��i<0 �ȴ������ӽ�����ֹ��
 * ����쳣��ֹ�����ӡ�����Ϣ
 */
int pwait(pid_t i)
{
	pid_t p;
	int e = 0, s;

	if(i == 0)
		return 0;
	for(;;) {
		p = wait(&s);
		if(p == -1) /* û���ӽ�����Ҫ�ȴ��� */
			break;
		e = WTERMSIG(s); /* ȡ�ӽ�����ֹ״̬ */
		if(e >= NSIGMSG || mesg[e] != NULL) { /*���ź���ֹ������Ӧ�������Ϣ*/
			if(p != i) {
				prn(p);
				prs(": ");
			}
			if (e < NSIGMSG)
				prs(mesg[e]);
			else {
				prs("Signal ");
				prn(e);
			}
		/*	if(WCOREDUMP(s))
		 *		prs(" -- Core dumped"); */
			err("");
		}
		if(i == p) /* ��ֹ���ӽ�����Ҫ�ȴ����ӽ��� */
			break;
	}
	if (WIFEXITED(s))
		return WEXITSTATUS(s);
	else
		return e | 0x80; /* �쳣��ֹ���ش��� 128 �� 8 λ���� */ 
}

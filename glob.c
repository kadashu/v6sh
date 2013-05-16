/* 
 *  globals params
 *
 *  �� params �е� "*" ƥ��������ʽ ".*"
 *  �� params �е� "?" ƥ��������ʽ "."
 *  �� params �е� "[...]" ƥ���ַ���
 *  �� params �е� "[!...]" ƥ���ַ���Ĳ�����
 *  �� params �е� "[...a-z...]" ƥ�� a �� z��
 *
 *  ִ��������а����¹�����Ĳ����б�:
 *      ��� param ������ "*"��"["���� "?"����ԭ��ʹ����
 *      ������������ڵ�ǰĿ¼���ҵ�ƥ����� param �������ļ���
 *      ����ʹ�����ǡ�
 */

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <setjmp.h>

#define	QUOTE 0x80	/* ���ñ�־λ */

#define	STRSIZ _POSIX_ARG_MAX
static char ab[STRSIZ];	/* ���ɵ��ַ����Ĵ洢�ռ� */
static char *string;	
static char *ava[STRSIZ/2];	/* ���ɵĲ����б� */
static char **av;
static int  ncoll;
static jmp_buf env; 

int glob(int argc, char *argv[]);
extern int texec(char* f, char **t);

static void expand(char *as);
static void sort(char **oav);
static void toolong(void);
static int match(char *s, char *p);
static int amatch(char *as, char *ap);
static int umatch(char *s, char *p);
static int compare(char *as1, char *as2);
static char* cat(char *as1, char *as2);

int glob(int argc, char *argv[])
{
	string = ab;
	av = ava;

	if (argc < 2) {
		write(STDERR_FILENO, "Arg count\n", 10);
		return 1;
	}
	argv++;
	*av++ = *argv; /* ָ���һ������������Ҫִ�е��ļ������� */

	if (setjmp(env) != 0)
		return 1;
	while (--argc >= 2) 
		expand(*++argv);  /* չ�����µ����еĲ��� */
	return texec(ava[0], ava);
}

void expand(char *as)
{
	register char *s, *cs;
	struct dirent * dirp; 
	DIR	*dp;
	char **oav;

	ncoll = 0;
	s = cs = as;
	/* �� cs ��λ����һ��ͨ��� */
	while (*cs!='*' && *cs!='?' && *cs!='[') {
		if (*cs++ == 0) { /* û���ҵ�ͨ��� */
			*av++ = cat(s, "");
			return; 
		}
	}
	for (;;) {
		if (cs==s) { /* ��ͨ���֮ǰû��'/' */
			dp = opendir(".");
			s = "";
			break;
		}
		if (*--cs == '/') {
			*cs = 0; /* �Ѳ����ֿ�ΪĿ¼���ļ��������ַ��� */
			dp = opendir((s==cs)? "/": s);
			*cs++ = QUOTE;	/* ����ǣ��ں���� cat �����лָ���б�� */
			break;
		}
	}
	if (dp == NULL) {
		write(STDERR_FILENO, "No directory\n", 13);
		longjmp(env,1);
	}
	oav = av;
	while ((dirp = readdir(dp)) != NULL) {
		if (match(dirp->d_name, cs)) {
			*av++ = cat(s, dirp->d_name);
			ncoll++;
		}
	}
	if (!ncoll) /* û��ƥ�� */
		*av++ = cat(s, cs); /* ���ֲ���Ϊԭ״ */
	else /* ����ƥ����ļ����� */
		sort(oav);
	closedir(dp);

}
/* ���򣬲��ò��������㷨  */
void sort(char **oav)
{
	register char **p1, **p2, *c;

	p1 = oav;
	while (++p1 < av) {
		c = *p1;
		p2 = p1;
		while (--p2 >= oav && compare(*p2, c) > 0)
			*(p2+1) = *p2;
		*(p2+1) = c;
	}
}

/* ��ӡ������Ϣ���˳� */
void toolong(void)
{
	write(2, "Arg list too long\n", 18);
	longjmp(env,1);
}
/*ƥ���ж����� */
int match(char *s, char *p)
{
	if (*s=='.' && *p!='.') /* �� . ��ͷ���ļ��Ĵ��� */
		return 0;
	return amatch(s, p);
}
/* ���ַ���ƥ�� */
int amatch(char *as, char *ap)
{
	register char *s, *p;
	register int scc;
	int c, cc, ok, lc;
	int neg = 0;

	s = as;
	p = ap;
	if ((scc = *s++) != 0) /* �ļ���δ�ս� */
		/* ���ݸ� glob �Ĳ����е��ַ���������������λ��
		��Щ�ַ��� shell ��������λ��" "��' ' �� */
		if ((scc &= ~QUOTE) == 0) /* ��� scc ���������λ֮���� 0 */
			scc = QUOTE; /* ���°�������Ϊ QUOTE */
	switch (c = *p++) {

	case '[':	/* �����ַ��� */
		if (*p=='!') { /* �¸��ַ��� ! */
			neg = 1;
			p++;
		}
		ok = 0;
		lc = INT_MAX;
		while ((cc = *p++) != 0) {
			if (cc==']') {
				if ((ok && !neg) || (!ok && neg)) 
					return amatch(s, p); /* ���ƥ�䣬�����ȽϺ�����ַ� */
				else
					return 0;
			} else if (cc=='-') {
				if (lc<=scc && scc<=(c = *p++)) /* �ڷ�Χ�� */
					ok++;
			} else
				if (scc == (lc=cc))
					ok++;
		}
		return 0;

	default:
		if (c!=scc)
			return 0;

	case '?':
		if (scc)
			return amatch(s, p);
		return 0;

	case '*':
		return umatch(--s, p);

	case '\0':
		return !scc;
	}
}
/* �Ահ���ƥ�� */
int umatch(char *s, char *p)
{
	if(*p==0) /* ģʽ�� * ���� */
		return 1;
	while(*s) /* �ļ��������ַ�ƥ��ģʽ�� * ������ַ� */
		if (amatch(s++,p))
			return 1;
	return 0;
}
/* �Ƚ������ַ��� */
int compare(char *as1, char *as2)
{
	register char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ ==  *s2)
		if (*s2++ == 0)
			return 0;
	return (*--s1 - *s2);
}
/* ���������ַ����� string ָ��������в���������
 * �������� string ָ�����ƣ�ָ�������ռ��С */
char* cat(char *as1, char *as2)
{
	register char *s1, *s2;
	register int c;

	s2 = string;
	s1 = as1;
	while ((c = *s1++) != 0) {
		if (s2 > &ab[STRSIZ])
			toolong();
		c &= ~QUOTE; /* �������λ */
		if (c==0) { /* ԭ����·���ָ��� */
			*s2++ = '/'; 
			break;
		}
		*s2++ = c;
	}

	s1 = as2;
	do {
		if (s2 > &ab[STRSIZ])
			toolong();
		*s2++ = c = *s1++;
	} while (c);

	s1 = string;
	string = s2;
	return s1;
}

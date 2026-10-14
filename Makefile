PROG_CXX=	stund
SRCS=		main.cpp network.cpp stun.cpp
MAN=

CXXSTD=		c++23
LDADD+=		-pthread

PREFIX?=	/usr/local
BINDIR?=	${PREFIX}/sbin

MK_DEBUG_FILES=	no
WITH_RC_SUBR?=	yes

.if ${WITH_RC_SUBR} == "yes"
FILES=			stund.sh
FILESNAME_stund.sh=	stund
FILESDIR=		${PREFIX}/etc/rc.d
FILESMODE=		0555

CLEANFILES+=	stund.sh

stund.sh: stund.in
	sed -e 's|%%PREFIX%%|${PREFIX}|g' ${.ALLSRC} > ${.TARGET}
.endif

.include <bsd.prog.mk>

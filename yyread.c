/* $Id: yyread.c,v 1.2 1991/11/03 21:53:20 piet Rel $ */
/* sozobon version */

#include <string.h>
#include <unistd.h>

/*
 * read, ignoring CR's
 *
 *  ++jrb
 */
int _yyread(fd, buf, size)
int fd; char *buf; int size;
{
    int count = read(fd, buf, size);
    int done = 0, i;

    if(count <= 0)
	return count;

    do{
	for(i = done; i < (done+count); i++)
	{
	    if(buf[i] == '\r')
	    {
		if(i < done + count - 1)
		  memmove(&buf[i], &buf[i+1], (count -1 - (i - done)));
		count -= 1;
	    }
	}
	done += count;
	if(done == size)
	    return done;
	count = read(fd, &buf[done], (size - done));
    } while(count > 0);

    return done;
}


How to Contribute to the Project
===============================================================================

This document is intended to act as a guide to help you contribute to the
project.  It is not perfect, and there will always be exceptions to the rules
described here, but by following the instructions below you should have a much
easier time getting your work merged upstream.

## Interacting with the Community

> "Be excellent to each other." - *Bill S. Preston, Esq.*

This project aims to be a welcoming place and we ask that anyone who interacts
with the project, and the greater community, treat each other with dignity and
respect.  Individuals who do not behave in such a manner will be warned and
asked to adjust their behavior; in extreme cases the individual may be blocked
from the project.

Examples of inappropriate behavior includes: profane, abusive, or prejudicial
language directed at another person, vandalism (e.g. GitHub issue/PR "litter"),
or spam.

## Explain Your Work

At the top of every patch and pull request you should include a description of
the problem you are trying to solve, how you solved it, and why you chose the
solution you implemented.  If you are submitting a bug fix, it is also
incredibly helpful if you can describe/include a reproducer for the problem in
the description as well as instructions on how to test for the bug and verify
that it has been fixed.

## Sign Your Work

The sign-off is a simple line at the end of the patch description, which
certifies that you wrote it or otherwise have the right to pass it on as an
open-source patch.  The "Developer's Certificate of Origin" pledge is taken
from the Linux Kernel and the rules are pretty simple:

	Developer's Certificate of Origin 1.1

	By making a contribution to this project, I certify that:

	(a) The contribution was created in whole or in part by me and I
	    have the right to submit it under the open source license
	    indicated in the file; or

	(b) The contribution is based upon previous work that, to the best
	    of my knowledge, is covered under an appropriate open source
	    license and I have the right under that license to submit that
	    work with modifications, whether created in whole or in part
	    by me, under the same open source license (unless I am
	    permitted to submit under a different license), as indicated
	    in the file; or

	(c) The contribution was provided directly to me by some other
	    person who certified (a), (b) or (c) and I have not modified
	    it.

	(d) I understand and agree that this project and the contribution
	    are public and that a record of the contribution (including all
	    personal information I submit with it, including my sign-off) is
	    maintained indefinitely and may be redistributed consistent with
	    this project or the open source license(s) involved.

... then you just add a line to the bottom of your patch description, with
your real name, saying:

	Signed-off-by: Random J Developer <random@developer.example.org>

You can add this to your commit description in `git` with `git commit -s`

## Submitting Your Patches Upstream

See [this guide](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/creating-a-pull-request)
if you've never submitted patches to a project via the GitHub pull request
mechanism.

#!/usr/bin/sh

# fetch external references
# NOTE: the `archive` operation uses SSH protocol, so please setup your SSH keys first.

git_server=192.168.10.6

# cvclient
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD cvdaemon/include/cvd/     | tar -x --strip=2 -C cvclient/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/bsw.git HEAD osa/include/osa/          | tar -x --strip=2 -C cvclient/include/
                                                                                                          
# cvdaemon                                                                                                
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD dscv/include/dscv/        | tar -x --strip=2 -C cvdaemon/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD face/include/face/        | tar -x --strip=2 -C cvdaemon/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD mio/include/mio/          | tar -x --strip=2 -C cvdaemon/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/bsw.git HEAD osa/include/osa/          | tar -x --strip=2 -C cvdaemon/include/
                                                                                                          
# dscv                                                                                                    
git archive --format=tar --remote=ssh://git@${git_server}/papillon/bsw.git HEAD osa/include/osa/          | tar -x --strip=2 -C dscv/include/
                                                                                                          
# face                                                                                                    
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD dscv/include/dscv/        | tar -x --strip=2 -C face/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/bsw.git HEAD osa/include/osa/          | tar -x --strip=2 -C face/include/
                                                                                                          
# mediac
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD dscv/include/dscv/        | tar -x --strip=2 -C mediac/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD mediad/include/mediad/    | tar -x --strip=2 -C mediac/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD mio/include/mio/          | tar -x --strip=2 -C mediac/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/bsw.git HEAD osa/include/osa/          | tar -x --strip=2 -C mediac/include/

# mediad
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD dscv/include/dscv/        | tar -x --strip=2 -C mediad/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD mio/include/mio/          | tar -x --strip=2 -C mediad/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/bsw.git HEAD osa/include/osa/          | tar -x --strip=2 -C mediad/include/

# mio
git archive --format=tar --remote=ssh://git@${git_server}/papillon/cv.git  HEAD dscv/include/dscv/        | tar -x --strip=2 -C mio/include/
git archive --format=tar --remote=ssh://git@${git_server}/papillon/bsw.git HEAD osa/include/osa/          | tar -x --strip=2 -C mio/include/


# do anything else

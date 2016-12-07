#Project 5 Filesystems 

## Required Project 2: Userprogs | All tests passing

## Initial Steps

 * Change line 363 in the file Pintos.pm in the utils folder from this:

		$name = find_file ("/home/ramesh/EE461S/SKKU/pintos/userprog/build/loader.bin") if !defined $name;

	* To this: 

			$name = find_file(‘<INSERT PATH TO FILESYS BUILD FOLDER>/loader.bin’) if !defined $name;
 * Change line 260 in the file pintos in the utils folder from this: 

		my $name =  "/home/ramesh/EE461S/SKKU/pintos/userprog/build/kernel.bin";

	* To this: 

			my $name =  "<INSERT PATH TO FILESYS BUILD FOLDER>/kernel.bin";


## Passing Tests
	pass tests/userprog/args-none
	pass tests/userprog/args-single
	pass tests/userprog/args-multiple
	pass tests/userprog/args-many
	pass tests/userprog/args-dbl-space
	pass tests/userprog/sc-bad-sp
	pass tests/userprog/sc-bad-arg
	pass tests/userprog/sc-boundary
	pass tests/userprog/sc-boundary-2
	pass tests/userprog/halt
	pass tests/userprog/exit
	pass tests/userprog/create-normal
	pass tests/userprog/create-empty
	pass tests/userprog/create-null
	pass tests/userprog/create-bad-ptr
	pass tests/userprog/create-long
	pass tests/userprog/create-exists
	pass tests/userprog/create-bound
	pass tests/userprog/open-normal
	pass tests/userprog/open-missing
	pass tests/userprog/open-boundary
	pass tests/userprog/open-empty
	pass tests/userprog/open-null
	pass tests/userprog/open-bad-ptr
	pass tests/userprog/open-twice
	pass tests/userprog/close-normal
	pass tests/userprog/close-twice
	pass tests/userprog/close-stdin
	pass tests/userprog/close-stdout
	pass tests/userprog/close-bad-fd
	pass tests/userprog/read-normal
	pass tests/userprog/read-bad-ptr
	pass tests/userprog/read-boundary
	pass tests/userprog/read-zero
	pass tests/userprog/read-stdout
	pass tests/userprog/read-bad-fd
	pass tests/userprog/write-normal
	pass tests/userprog/write-bad-ptr
	pass tests/userprog/write-boundary
	pass tests/userprog/write-zero
	pass tests/userprog/write-stdin
	pass tests/userprog/write-bad-fd
	pass tests/userprog/exec-once
	pass tests/userprog/exec-arg
	pass tests/userprog/exec-multiple
	pass tests/userprog/exec-missing
	pass tests/userprog/exec-bad-ptr
	pass tests/userprog/wait-simple
	pass tests/userprog/wait-twice
	pass tests/userprog/wait-killed
	pass tests/userprog/wait-bad-pid
	pass tests/userprog/multi-recurse
	pass tests/userprog/multi-child-fd
	pass tests/userprog/rox-simple
	pass tests/userprog/rox-child
	pass tests/userprog/rox-multichild
	pass tests/userprog/bad-read
	pass tests/userprog/bad-write
	pass tests/userprog/bad-read2
	pass tests/userprog/bad-write2
	pass tests/userprog/bad-jump
	pass tests/userprog/bad-jump2
	pass tests/filesys/base/lg-create
	pass tests/filesys/base/lg-full
	pass tests/filesys/base/lg-random
	pass tests/filesys/base/lg-seq-block
	pass tests/filesys/base/lg-seq-random
	pass tests/filesys/base/sm-create
	pass tests/filesys/base/sm-full
	pass tests/filesys/base/sm-random
	pass tests/filesys/base/sm-seq-block
	pass tests/filesys/base/sm-seq-random
	pass tests/filesys/base/syn-read
	pass tests/filesys/base/syn-remove
	pass tests/filesys/base/syn-write
	pass tests/filesys/extended/dir-empty-name
	pass tests/filesys/extended/dir-mk-tree
	pass tests/filesys/extended/dir-mkdir
	pass tests/filesys/extended/dir-open
	pass tests/filesys/extended/dir-over-file
	pass tests/filesys/extended/dir-rm-cwd
	pass tests/filesys/extended/dir-rm-parent
	pass tests/filesys/extended/dir-rm-root
	pass tests/filesys/extended/dir-rm-tree
	pass tests/filesys/extended/dir-rmdir
	pass tests/filesys/extended/dir-under-file
	FAIL tests/filesys/extended/dir-vine
	pass tests/filesys/extended/grow-create
	pass tests/filesys/extended/grow-dir-lg
	pass tests/filesys/extended/grow-file-size
	pass tests/filesys/extended/grow-root-lg
	pass tests/filesys/extended/grow-root-sm
	pass tests/filesys/extended/grow-seq-lg
	pass tests/filesys/extended/grow-seq-sm
	pass tests/filesys/extended/grow-sparse
	pass tests/filesys/extended/grow-tell
	pass tests/filesys/extended/grow-two-files
	pass tests/filesys/extended/syn-rw
	FAIL tests/filesys/extended/dir-empty-name-persistence
	FAIL tests/filesys/extended/dir-mk-tree-persistence
	FAIL tests/filesys/extended/dir-mkdir-persistence
	FAIL tests/filesys/extended/dir-open-persistence
	FAIL tests/filesys/extended/dir-over-file-persistence
	FAIL tests/filesys/extended/dir-rm-cwd-persistence
	FAIL tests/filesys/extended/dir-rm-parent-persistence
	FAIL tests/filesys/extended/dir-rm-root-persistence
	FAIL tests/filesys/extended/dir-rm-tree-persistence
	FAIL tests/filesys/extended/dir-rmdir-persistence
	FAIL tests/filesys/extended/dir-under-file-persistence
	FAIL tests/filesys/extended/dir-vine-persistence
	FAIL tests/filesys/extended/grow-create-persistence
	FAIL tests/filesys/extended/grow-dir-lg-persistence
	FAIL tests/filesys/extended/grow-file-size-persistence
	FAIL tests/filesys/extended/grow-root-lg-persistence
	FAIL tests/filesys/extended/grow-root-sm-persistence
	FAIL tests/filesys/extended/grow-seq-lg-persistence
	FAIL tests/filesys/extended/grow-seq-sm-persistence
	FAIL tests/filesys/extended/grow-sparse-persistence
	FAIL tests/filesys/extended/grow-tell-persistence
	FAIL tests/filesys/extended/grow-two-files-persistence
	FAIL tests/filesys/extended/syn-rw-persistence
	24 of 121 tests failed.

	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	SUMMARY BY TEST SET

	Test Set                                      Pts Max  % Ttl  % Max
	--------------------------------------------- --- --- ------ ------
	tests/filesys/extended/Rubric.functionality    29/ 34  25.6%/ 30.0%
	tests/filesys/extended/Rubric.robustness       10/ 10  15.0%/ 15.0%
	tests/filesys/extended/Rubric.persistence       0/ 23   0.0%/ 20.0%
	tests/filesys/base/Rubric                      30/ 30  20.0%/ 20.0%
	tests/userprog/Rubric.functionality           108/111   9.7%/ 10.0%
	tests/userprog/Rubric.robustness               88/ 88   5.0%/  5.0%
	--------------------------------------------- --- --- ------ ------
	Total                                                  75.3%/100.0%

	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	 








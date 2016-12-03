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
	FAIL tests/userprog/create-exists
	pass tests/userprog/create-bound
	FAIL tests/userprog/open-normal
	pass tests/userprog/open-missing
	FAIL tests/userprog/open-boundary
	pass tests/userprog/open-empty
	pass tests/userprog/open-null
	pass tests/userprog/open-bad-ptr
	FAIL tests/userprog/open-twice
	FAIL tests/userprog/close-normal
	FAIL tests/userprog/close-twice
	pass tests/userprog/close-stdin
	pass tests/userprog/close-stdout
	pass tests/userprog/close-bad-fd
	FAIL tests/userprog/read-normal
	FAIL tests/userprog/read-bad-ptr
	FAIL tests/userprog/read-boundary
	FAIL tests/userprog/read-zero
	pass tests/userprog/read-stdout
	pass tests/userprog/read-bad-fd
	FAIL tests/userprog/write-normal
	FAIL tests/userprog/write-bad-ptr
	FAIL tests/userprog/write-boundary
	FAIL tests/userprog/write-zero
	pass tests/userprog/write-stdin
	pass tests/userprog/write-bad-fd
	FAIL tests/userprog/exec-once
	FAIL tests/userprog/exec-arg
	FAIL tests/userprog/exec-multiple
	pass tests/userprog/exec-missing
	pass tests/userprog/exec-bad-ptr
	FAIL tests/userprog/wait-simple
	FAIL tests/userprog/wait-twice
	FAIL tests/userprog/wait-killed
	pass tests/userprog/wait-bad-pid
	FAIL tests/userprog/multi-recurse
	FAIL tests/userprog/multi-child-fd
	FAIL tests/userprog/rox-simple
	FAIL tests/userprog/rox-child
	FAIL tests/userprog/rox-multichild
	pass tests/userprog/bad-read
	pass tests/userprog/bad-write
	pass tests/userprog/bad-read2
	pass tests/userprog/bad-write2
	pass tests/userprog/bad-jump
	pass tests/userprog/bad-jump2
	FAIL tests/filesys/base/lg-create
	FAIL tests/filesys/base/lg-full
	FAIL tests/filesys/base/lg-random
	FAIL tests/filesys/base/lg-seq-block
	FAIL tests/filesys/base/lg-seq-random
	FAIL tests/filesys/base/sm-create
	FAIL tests/filesys/base/sm-full
	FAIL tests/filesys/base/sm-random
	FAIL tests/filesys/base/sm-seq-block
	FAIL tests/filesys/base/sm-seq-random
	FAIL tests/filesys/base/syn-read
	FAIL tests/filesys/base/syn-remove
	FAIL tests/filesys/base/syn-write
	FAIL tests/filesys/extended/dir-empty-name
	FAIL tests/filesys/extended/dir-mk-tree
	FAIL tests/filesys/extended/dir-mkdir
	FAIL tests/filesys/extended/dir-open
	FAIL tests/filesys/extended/dir-over-file
	FAIL tests/filesys/extended/dir-rm-cwd
	FAIL tests/filesys/extended/dir-rm-parent
	FAIL tests/filesys/extended/dir-rm-root
	FAIL tests/filesys/extended/dir-rm-tree
	FAIL tests/filesys/extended/dir-rmdir
	FAIL tests/filesys/extended/dir-under-file
	FAIL tests/filesys/extended/dir-vine
	FAIL tests/filesys/extended/grow-create
	FAIL tests/filesys/extended/grow-dir-lg
	FAIL tests/filesys/extended/grow-file-size
	FAIL tests/filesys/extended/grow-root-lg
	FAIL tests/filesys/extended/grow-root-sm
	FAIL tests/filesys/extended/grow-seq-lg
	FAIL tests/filesys/extended/grow-seq-sm
	FAIL tests/filesys/extended/grow-sparse
	FAIL tests/filesys/extended/grow-tell
	FAIL tests/filesys/extended/grow-two-files
	FAIL tests/filesys/extended/syn-rw
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

	84 of 121 tests failed.

 








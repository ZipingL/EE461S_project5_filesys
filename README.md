 Project2 - Process API and SysCalls

This is the [User Programs][userprog] project in Pintos. To get started:

- Form a team of 3 members

1. Create a fork of this project2 repository (click on the "Fork" button above)
1. Give your team write permissions under Settings -> Collaborators and Teams.
1. Working on the Project on your Linux Machine:
  2. [**Clone**][ref-clone] the repository to your computer.
  2. As you work on the project, you will modify the files and [**commit**][ref-commit] changes to complete your solution.
  2. [**Push**][ref-push]/sync the changes up to your fork on GitHub.
1. Submitting your Project:
  2. [Create a **pull request**][pull-request] on the original repository to turn in the assignment.

- Tests Fail/Pass Status (Update this every time you push with what you see in make check,
please never push if you cause tests to fail that weren't failing already)

#TEST STATUS
- FAIL tests/userprog/no-vm/multi-oom
No idea whats going on with this one^
- All test cases working (except for the sporadic case above, but that's just extra credit anyway)
- Writing the wiki page to document the project

#Project 3
- Read about the project here: https://web.stanford.edu/class/cs140/projects/pintos/pintos_4.html
- Suggested Order of Implementation:
	- Frame table (without swapping). Once complete, all project 2 test cases should still be passing
	- Supplemental Page Table
	- Page fault handler. Kernel should pass all projecct 2 test cases, but only some of the robustness cases (?)
	- Stack growth
	- Mapped files
	- Page reclamation on process exit (the last three can be done in parallel)
	- Eviction
- I would focus on trying the last couple of problems on the practice exam, because it really does give you a good idea of how paging works and so would really clear up your understanding before you just start writing code.

<!-- Links -->
[userprog]: https://web.stanford.edu/class/cs140/projects/pintos/pintos_3.html#SEC32
[forking]: https://guides.github.com/activities/forking/
[ref-clone]: http://gitref.org/creating/#clone
[ref-commit]: http://gitref.org/basic/#commit
[ref-push]: http://gitref.org/remotes/#push
[pull-request]: https://help.github.com/articles/creating-a-pull-request
[raw]: https://raw.githubusercontent.com/education/guide/master/docs/forks.md

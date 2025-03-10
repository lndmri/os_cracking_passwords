# os_cracking_passwords

## Quick Start

1. Open a terminal and clone the repository

```bash
git clone git@github.com:lndmri/os_cracking_passwords.git
```

## Contributing

Each teammate has an issue assigned to work. Go to the issues page. 
For each issue each teammate will create its branch to work on it.

1. Create a working branch

```bash
# checkout the main branch
git checkout main
# make sure you have the latest version
git pull
# create a new working branch for an incremental change
# github issues lets you copy/paste a good branch name
git checkout -b my-incremental-change  
```

2. Make some changes (repeat as needed)

```bash
# stage some changed files
git add [file_name(s)]
# or stage all your changed files
git add .
# or use your preferred git GUI (VSCode, GH Desktop, etc.) to select what to stage

# use commit to commit the changes
git commit -m "Type a short message here"
```

3. Once you're confident everything is working as it should

```bash
# push your changes
git push
# after pushing, the CLI will give you a link to open a pull request
# or you can go to github and create one manually
```

5. Request code review from another developer

6. If changes are requested, return to step 2 to fix

7. After the pull request is approved, click the **Rebase and Merge** button, this is to merge to main.


Congratulations, you're done! Now you can select the next GitHub issue you'd like to work on and start again from step 1.
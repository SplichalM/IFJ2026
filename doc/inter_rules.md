
## Versioning

This project follows Semantic Versioning: MAJOR.MINOR.PATCH.

Versioning is applied only in the `main` branch.
The `develop` branch does not maintain official version numbers.

- MAJOR is increased only when the functionality is completely changed and the new version is not compatible with the previous one.
- MINOR is increased when a larger change is introduced or when a completely new functionality is added.
- PATCH is increased for all other smaller upgrades, improvements, and changes.


## Commit Convention

All commits must follow a structured format consisting of three parts:

`<type>[optional scope]: <short summary>`

`- list of changes or additional important information`

`BREAKING CHANGE: explanation (only if needed)`


### Structure Explanation

1. Header (required)

Format:
`<type>[optional scope]: <short summary>`

- `type` defines the category of change.
- `scope` is optional, but must be used if changes were made outside the expected topic of the feature branch.
- `short summary` is a clear and concise description of the commit.

The header must always be a single line.


2. Body (required)

After a blank line, provide:

`- A point list describing what was changed and any additional context or relevant information.`

The body must clearly describe the changes in a structured and readable way.


3. Footer (only if necessary)

The footer is used only when the commit introduces breaking changes.

It must clearly explain:
- What is no longer compatible.
- What must be changed elsewhere in the codebase.

Format:
`BREAKING CHANGE: explanation of incompatibility`

This section is required only if the change breaks existing functionality.


### Allowed Commit Types

We use the following commit types:

- `feat` – new functionality, upgrades, code extensions.
- `fix` – bug fixes and error corrections.
- `docs` – documentation changes.
- `style` – formatting or stylistic changes without functional impact.
- `refactor` – code restructuring or renaming without changing functionality.
- `perf` – performance optimizations without changing functionality.
- `test` – adding or modifying tests.
- `chore` – maintenance, configuration, and project management changes.
  This type will be used primarily by the project maintainer.


### Example Commit

```
feat(auth): replace legacy authentication system

- implemented new JWT-based authentication
- removed old session-based login handling
- updated authentication middleware

BREAKING CHANGE: legacy session authentication has been removed. All services depending on session tokens must be updated to use JWT.
```

## Branch Structure

This repository uses the following branch structure and rules.

### main
- Used only for main versions we want to keep (release-ready states).
- No direct coding happens in this branch.

### develop
- Used as an integration branch for merging all feature/fix/other branches.
- No direct coding happens in this branch.

### feature/*
- Real development happens here.
- Each developer works under their own namespace based on xlogin.

Branch pattern:
- `feature/<topic>/<feature-name>`

Examples:
- `feature/lexer/auth-refresh-token`
- `feature/symtable/ui-navbar`

### hotfix/*
- Used for small code fixes (usually urgent).
- These branches contain real coding.

Branch pattern:
- `hotfix/<issue-or-short-name>`

Example:
- `hotfix/login-null-check`

### release/*
- Used for preparing releases (stabilization, final checks, packaging).
- Real work can happen here, but it should be focused on getting a release ready.

Branch pattern:
- `release/<version>`

Example:
- `release/1.4.2`

#### Merge Rules (Important)
- Everyone can merge their finished `release/*` branches into `develop` when appropriate.
- Avoid touching the process of merging `develop` into `main`. 

---

### Branch Hierarchy Diagram

```
main
develop
feature/
  └── <topic>/
      ├── <feature-name>
      └── <feature-name>
hotfix/
  ├── <issue-or-short-name>
  └── <issue-or-short-name>
release/
  ├── <version>
  └── <version>
```
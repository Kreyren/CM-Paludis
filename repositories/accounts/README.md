Function used to handle system users and user groups

---

# Accounts

Paludis is expected to handle accounts (system users and user groups) through downstream repositories for distro-oriented handling of expected accounts.

Accounts definitions are expected in `REPOSITORY/metadatad/accounts/{user,group}/foo.conf`

User `git` with home directory `/var/git` using shell `/bin/nologin` part of group `git` is defined as:

```conf
gecos = Git user
home = /var/git
shell = /bin/nologin
primary_group = git
```

Group `mail` with GID 12 is defined as:

```conf
preferred_gid = 12
```
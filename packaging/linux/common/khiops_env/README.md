# khiops_env generation

khiops_env is built by cmake at configuration time by replacing variables in the file khiops_env.in. These variables are surrounded by '@'.
Some variables contain bash lines so to improve readability, they are filled from the files:
- export_env_variables.sh
- java_settings.sh
- use_environment_module.sh.in

@echo "deploy shv" %WORKSPACE%

IF NOT EXIST \\claudius.elektroline.cz\install\shvspy\%CI_COMMIT_REF_SLUG% mkdir \\claudius.elektroline.cz\install\shvspy\%CI_COMMIT_REF_SLUG%
copy _inno\shvspy\* \\claudius.elektroline.cz\install\shvspy\%CI_COMMIT_REF_SLUG%   || exit /b 2
@echo "copied."

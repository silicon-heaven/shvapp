@echo "deploy shv" %WORKSPACE%

@echo "copying" _inno\shvspy\* to \\claudius.elektroline.cz\install\shvspy\%CI_COMMIT_REF_SLUG%

IF NOT EXIST \\claudius.elektroline.cz\install\shvspy\%CI_COMMIT_REF_SLUG% mkdir \\claudius.elektroline.cz\install\shvspy\%CI_COMMIT_REF_SLUG%
copy _inno\shvspy\* \\claudius.elektroline.cz\install\shvspy\%CI_COMMIT_REF_SLUG%   || exit /b 2

@echo "copying" _inno\bfsview\* to \\claudius.elektroline.cz\install\bfsview\%CI_COMMIT_REF_SLUG%

IF NOT EXIST \\claudius.elektroline.cz\install\bfsview\%CI_COMMIT_REF_SLUG% mkdir \\claudius.elektroline.cz\install\bfsview\%CI_COMMIT_REF_SLUG%
copy _inno\bfsview\* \\claudius.elektroline.cz\install\bfsview\%CI_COMMIT_REF_SLUG%   || exit /b 2

@echo "copied."

echo "deploy shv" ${WORKSPACE}
INSTALL_DIR=//claudius.elektroline.cz/install/shv/${CI_COMMIT_REF_SLUG}
[ -d ${INSTALL_DIR} ] || mkdir ${INSTALL_DIR}
cp _inno/shv/* ${INSTALL_DIR} || exit 2
echo "copied."

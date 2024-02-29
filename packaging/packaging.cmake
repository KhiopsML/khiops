set(CPACK_PACKAGE_VENDOR Orange)
set(CPACK_PACKAGE_HOMEPAGE_URL https://khiops.org)
set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/packaging/common/images/khiops.png")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VENDOR "Orange")
set(CPACK_PACKAGE_CONTACT "Khiops Team <khiops.team@orange.com>")
set(CPACK_SOURCE_IGNORE_FILES .git)

set(CPACK_COMPONENT_KHIOPS_DESCRIPTION
    "software for data mining
Khiops is a data preparation and scoring tool for supervised learning
and unsupervised learning. It allows one to perform univariate and
bivariate descriptive statistics, to evaluate the predictive
importance of explanatory variables, to discretize numerical
variables, to group the values of categorical variables, to recode
input data according to these discretizations and value groupings. It
allows one to perform multi-table relational mining with automatic
variable construction. Khiops also produces a scoring model for
supervised learning tasks, according to a Selective Naive Bayes
approach, either for classification or for regression.  Khiops is
available both in user interface mode and in batch mode, such that it
can easily be embedded as a software component in a data mining
deployment projet.")

set(CPACK_COMPONENT_KHIOPS_CORE_DESCRIPTION
    "software for data mining
Khiops is a data preparation and scoring tool for supervised learning and
unsupervised learning. See the khiops package for more information.
 .
This package is a minimal Khiops package, it contains the minimal files and
dependancies to use Khiops. It is intended to be used on servers : it comes
without GUI, samples or documentation. The full Khiops distribution is
available with the 'khiops' package.")

set(CPACK_COMPONENT_KNI_DESCRIPTION
    "Khiops Native Interface
The purpose of Khiops Native Interface (KNI) is to allow a deeper integration
of Khiops in information systems, by mean of the C programming language, using
a dynamic link library (DLL). This relates specially to the problem of model
deployment, which otherwise requires the use of input and output data files
when using directly the Khiops tool in batch mode. See Khiops Guide for an
introduction to dictionary files, dictionaries, database files and deployment.
 .
The Khiops deployment features are thus made public through an API with a DLL.
Therefore, a Khiops model can be deployed directly from any programming
language, such as C, C++, Java, Python, Matlab, etc. This enables real time
model deployment without the overhead of temporary data files or launching
executables. This is critical for certain applications, such as marketing or
targeted advertising on the web.
 .
All KNI functions are C functions for easy use with other programming languages.
They return a positive or zero value in case of success, and a negative error
code in case of failure. The functions are not reentrant (thread-safe): the DLL
can be used simultaneously by several executables, but not simultaneously by
several threads in the same executable.
 .
See KhiopsNativeInterface.h for a detailed description of KNI functions")

set(CPACK_COMPONENT_KNI_DOC_DESCRIPTION "Documentation and examples for Khiops Native Interface (kni)")

set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

# ########### ARCHIVE Generator #############################

# One package per component
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# user friendly archive names
set(CPACK_ARCHIVE_KNI_FILE_NAME kni-${CMAKE_PROJECT_VERSION})
set(CPACK_ARCHIVE_KNI_DOC_FILE_NAME kni-doc-${CMAKE_PROJECT_VERSION})
set(CPACK_ARCHIVE_KHIOPS_FILE_NAME khiops-${CMAKE_PROJECT_VERSION})
set(CPACK_ARCHIVE_KHIOPS_CORE_FILE_NAME khiops-core-${CMAKE_PROJECT_VERSION})

# ########### DEB Generator #############################

# Package release ("This is the numbering of the DEB package itself, i.e. the version of the packaging and not the
# version of the content")
set(CPACK_DEBIAN_PACKAGE_RELEASE 1)

# package name for deb.
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_DEB_COMPONENT_INSTALL YES)
set(CPACK_DEBIAN_PACKAGE_SECTION "math")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

# packages names
set(CPACK_DEBIAN_KHIOPS_PACKAGE_NAME khiops)
set(CPACK_DEBIAN_KHIOPS_CORE_PACKAGE_NAME khiops-core)
set(CPACK_DEBIAN_KNI_PACKAGE_NAME kni)
set(CPACK_DEBIAN_KNI_DOC_PACKAGE_NAME kni-doc)

# packages depends
set(CPACK_DEBIAN_KHIOPS_CORE_PACKAGE_DEPENDS "mpich (>= 3.0)")
set(CPACK_DEBIAN_KHIOPS_PACKAGE_DEPENDS
    "khiops-core (=${CMAKE_PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}), default-jre (>=1.8)")

# packages recommends
set(CPACK_DEBIAN_KHIOPS_CORE_PACKAGE_RECOMMENDS "khiops, khiops-visualization")
set(CPACK_DEBIAN_KHIOPS_KNI_RECOMMENDS kni-doc)

# packages posinst and triggers
set(CPACK_DEBIAN_KHIOPS_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/packaging/linux/debian/khiops/postinst")
set(CPACK_DEBIAN_KHIOPS_CORE_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/packaging/linux/debian/khiops-core/postinst")
set(CPACK_DEBIAN_KNI_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/packaging/linux/debian/kni/triggers")

# set(CPACK_DEBIAN_PACKAGE_DEBUG ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)

# ########### RPM Generator #############################

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_LICENSE BSD-3-Clause) # TODO TO CHECK
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_RPM_PACKAGE_VENDOR Orange)

set(CPACK_RPM_KHIOPS_PACKAGE_AUTOREQ ON)

# packages names
set(CPACK_RPM_KHIOPS_PACKAGE_NAME khiops)
set(CPACK_RPM_KHIOPS_CORE_PACKAGE_NAME khiops-core)
set(CPACK_RPM_KNI_PACKAGE_NAME kni)
set(CPACK_RPM_KNI_DOC_PACKAGE_NAME kni-doc)

# default file name e.g. khiops-10.0.0-1.x86_64.rpm
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

# packages summary
set(CPACK_RPM_KHIOPS_PACKAGE_SUMMARY "Khiops tools")
set(CPACK_RPM_KHIOPS_CORE_PACKAGE_SUMMARY "Khiops tools (core)")
set(CPACK_RPM_KNI_PACKAGE_SUMMARY "Khiops Native Interface")
set(CPACK_RPM_KNI_DOC_PACKAGE_SUMMARY "Khiops Native Interface documentation")

# packages requires
set(CPACK_RPM_KHIOPS_PACKAGE_REQUIRES "khiops-core = ${CMAKE_PROJECT_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}")
set(CPACK_RPM_KHIOPS_PACKAGE_REQUIRES "java >= 1.8")
set(CPACK_RPM_KHIOPS_CORE_PACKAGE_REQUIRES "util-linux")

# packages post/postun install scripts
set(CPACK_RPM_KHIOPS_CORE_POST_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/packaging/linux/redhat/khiops-core.post")
set(CPACK_RPM_KHIOPS_POST_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/packaging/linux/redhat/khiops.post")
set(CPACK_RPM_KNI_POST_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/packaging/linux/redhat/kni.post")
set(CPACK_RPM_KNI_POSTUN_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/packaging/linux/redhat/kni.postun")

get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL "Unspecified")

include(CPack)

# Check if all files are installed within a component. If not, these files will be in the package
# Unspecified.zip/deb/rpm It doesn't work on vscode: Unspecified component is always here list(FIND CPACK_COMPONENTS_ALL
# ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME} _test) if (${_test} GREATER "-1") message(AUTHOR_WARNING "Installing files
# without component. These files won't be in any package. See file
# install_manifest_${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}.txt.") endif()

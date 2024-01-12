import os


def get_test_family(tool):
    """Return list of tes families per tool
    Account for 'KhiopsCompleteTests' env var for extended families"""
    # Khiops tool
    if tool == "Khiops":
        test_family = [
            "Standard",
            "SideEffects",
            "Rules",
            "MissingValues",
            "Advanced",
            "Bugs",
            "BugsMultiTables",
            "MultipleTargets",
            "MultiTables",
            "DeployCoclustering",
            "SparseData",
            "SparseModeling",
            "ParallelTask",
            "NewPriorV9",
            "DTClassification",
            "VariableConstruction",
            "NewV10",
            "KIInterpretation",
            "CrashTests",
            "SmallInstability",
        ]
        # V11        "Histograms",
        # V11        "HistogramsLimits",
        # V11        "TextVariables",
        # Following tests are very long, unstable and not useful:
        if os.getenv("KhiopsCompleteTests") == "true":
            test_family.append("Classification")
            test_family.append("MTClassification")
            test_family.append("Regression")
            test_family.append("ChallengeAutoML")
            # V11 test_family.append("TextClassification")

    # Coclustering tool
    if tool == "Coclustering":
        test_family = ["Standard", "Bugs", "NewPriorV9", "SmallInstability"]

    # KNI tool
    if tool == "KNI":
        test_family = ["Standard", "MultiTables", "SmallInstability"]
    return test_family

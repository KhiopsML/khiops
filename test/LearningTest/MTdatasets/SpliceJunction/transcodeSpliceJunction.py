import os


def Transcode(outputFileName, outputDataFileName):
    # ouverture des fichiers
    # fInput = open("splice.data", 'r')
    # avec les donnees sans doublons
    fInput = open("splice.data.cleaned", "r")
    fOutput = open(outputFileName, "w")
    fOutputData = open(outputDataFileName, "w")

    # ligne d'entete
    fOutput.write("SampleId\tClass\n")
    fOutputData.write("SampleId\tPos\tChar\n")

    # parcours de repertoires
    lines = fInput.readlines()
    for line in lines:
        line = line.replace("\n", "")
        line = line.replace(" ", "")
        fields = line.split(",")
        Class = fields[0]
        sampleId = fields[1]
        DNA = fields[2]
        fOutput.write(sampleId + "\t" + Class + "\n")
        index = 1
        for char in DNA:
            fOutputData.write(sampleId + "\t" + str(index) + "\t" + char + "\n")
            index = index + 1

    # fermeture  des fichiers
    fInput.close()
    fOutput.close()
    fOutputData.close()


Transcode("SpliceJunction.txt", "SpliceJunctionDNA.txt")

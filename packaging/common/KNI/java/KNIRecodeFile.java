// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import com.sun.jna.Library;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class KNIRecodeFile
{

        public final static int MAXBUFFERSIZE = 10000;

        /**
         * Get the KNI libray name according to the current os and platform
         */
        public final static String getLibraryName()
        {
                String os = System.getProperty("os.name").toLowerCase();
                String platform = System.getProperty("sun.arch.data.model");
                // Windows
                String libraryName = "";
                if (os.indexOf("win") >= 0)
                        libraryName = KNI.LIBRARY_WINDOWS_NAME_64;
                // Linux
                else if (os.indexOf("linux") >= 0) {
                        libraryName = KNI.LIBRARY_LINUX_NAME;
                }
                return libraryName;
        }

        /**
         * Recode an input file to an output file, using a Khiops dictionary
         * from a dictionary file
         * The input file must have a header line, describing the structure of all its instances
         * The input and output files have a tabular format
         *
         * @param dictionaryFileName Name of the dictionary file
         * @param dictionaryName Name of the dictionary
         * @param inputFileName Name of the input file name
         * @param outputFileName  Name of the output file name
         */
        private static void RecodeFile(String dictionaryFileName,
                                       String dictionaryName,
                                       String inputFileName,
                                       String outputFileName,
                                       String errorFileName)
        {

                int retCode;
                int streamNumber;
                int recordNumber;
                String headerLine = null;
                String inputRecord = null;
                Memory outputRecord = new Memory(MAXBUFFERSIZE);
                BufferedReader reader = null;
                BufferedWriter writer = null;

                if (dictionaryFileName == null || dictionaryName == null || inputFileName == null ||
                    outputFileName == null) {
                        throw new IllegalArgumentException("One parameter is not filled in.");
                }

                // Load KNI library
                String libraryName = getLibraryName();
                Library noThreadSafeLibrary = (Library)Native.loadLibrary(libraryName, KNI.class);
                KNI kni = (KNI)Native.synchronizedLibrary(noThreadSafeLibrary);

                // Start message
                System.out.printf("\nRecode records of %s to %s\n", inputFileName, outputFileName);
                recordNumber = 0;

                // Open input and output files
                try {
                        reader = new BufferedReader(new FileReader(inputFileName));
                } catch (FileNotFoundException e) {
                        System.err.println("Problem opening file: " + inputFileName);
                        e.printStackTrace();
                }
                try {
                        writer = new BufferedWriter(new FileWriter(outputFileName));
                } catch (IOException e1) {
                        System.err.println("Problem opening file: " + outputFileName);
                        e1.printStackTrace();
                }

                // Open KNI stream
                streamNumber = -1;

                if (reader != null && writer != null) {
                        // Read header line
                        try {
                                headerLine = reader.readLine();
                        } catch (IOException e) {
                                System.err.println("An error occured while reading file: " + inputFileName);
                                e.printStackTrace();
                        }

                        // Open stream
                        if (headerLine != null) {
                                streamNumber = kni.KNIOpenStream(dictionaryFileName, dictionaryName, headerLine, '\t');
                                if (streamNumber < 0)
                                        System.out.printf("\tOpen stream error: %d\n", streamNumber);
                        }
                }

                // Recode all records of the input file
                if (streamNumber >= 0) {
                        // Loop on input records
                        try {
                                while ((inputRecord = reader.readLine()) != null) {

                                        // Recode record
                                        retCode = kni.KNIRecodeStreamRecord(
                                          streamNumber, inputRecord, outputRecord, MAXBUFFERSIZE);

                                        // Write output record
                                        if (retCode == KNI.OK) {
                                                writer.write(outputRecord.getString(0));
                                                writer.newLine();
                                                recordNumber++;
                                        }
                                }
                        } catch (IOException e) {
                                System.err.println("An error occured while reading file: " + inputFileName);
                                e.printStackTrace();
                        }
                }

                // Close files
                if (reader != null)
                        try {
                                reader.close();
                        } catch (IOException e) {
                                System.err.println("An error occured while closing file: " + inputFileName);
                                e.printStackTrace();
                        }
                if (writer != null)
                        try {
                                writer.close();
                        } catch (IOException e) {
                                System.err.println("An error occured while closing file: " + outputFileName);
                                e.printStackTrace();
                        }

                // Close stream
                if (streamNumber >= 0)
                        kni.KNICloseStream(streamNumber);

                // End message
                System.out.printf("Recoded record number: %d\n", recordNumber);
        }

        /**
         * Entry point to recode a file using a dictionary
         * @param args [Dictionary file] [Dictionary] [Input file] [Output file]
         */
        public static void main(String[] args)
        {
                if (args.length != 4 && args.length != 5) {
                        // Usage
                        System.out.println("\t<Dictionary file> <Dictionary> <Input File> <Output File> [Error file]");
                } else {
                        // Case without an error file
                        if (args.length == 4)
                                RecodeFile(args[0], args[1], args[2], args[3], "");
                        // Case with an error file
                        else
                                RecodeFile(args[0], args[1], args[2], args[3], args[4]);
                }
        }
}

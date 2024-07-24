Reference:
- issue sur le githuv khiops: MPI crash on Debian/Ubuntu during classifier training #104 
- bug detecté par Nicolas dans un contexte multi-table, avec de nombreuse logne orphelines dans les tables secondaires

Bug se produisant dans les conditions suibantes:
- en parallele (trois coeurs par exemple)
- sur Debian 10 ou 11 (et Ubunto 22?)
- avec une nouvelle version de MPI (4.1)
- quand il y a beaucup d'erreur transmise entre les esclaves et le mettre

Des tests ont ete effectues sur la branche du repo clone par Stephane: Bug-MPI
- cf. sur repo clone de Stephane: Bug MPI traces 1
- Ajout de traces dans les methodes StartFileServers et StopFileServers de PLMPITaskDriver pour preciser le probleme
  - plante dans StopFileServers, au moment de l'appel a MPI_Barrier
  
Test minimaliste de Nicolas
- base Adult, en ayant change le type d'une variable Categorical a Numerical

Plante avec trace obtenue:
warning : Data table Adult.txt : Record 3 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 4 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 5 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 7 : Numerical variable sex: value <Female> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 8 : Numerical variable sex: value <Female> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 9 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 10 : Numerical variable sex: value <Female> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 11 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 14 : Numerical variable sex: value <Female> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 15 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 16 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 17 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 18 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 19 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 20 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 22 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 24 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 25 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 26 : Numerical variable sex: value <Female> converted to <> (Unconverted string)
warning : Data table Adult.txt : Record 27 : Numerical variable sex: value <Male> converted to <> (Unconverted string)
warning : Data table : ...
error : MPI driver : Other MPI error, error stack:
internal_Comm_disconnect(81)...: MPI_Comm_disconnect(comm=0x55fe5eead028) failed
MPID_Comm_disconnect(493)......:
MPIR_Comm_free_impl(809).......:
MPIR_Comm_delete_internal(1224): Communicator (handle=84000003) being freed has 1 unmatched message(s)
Abort(274287887) on node 1 (rank 1 in comm 0): application called MPI_Abort(MPI_COMM_WORLD, 274287887) - process 1
error : MPI driver : Other MPI error, error stack:
internal_Comm_disconnect(81)...: MPI_Comm_disconnect(comm=0x56352c201028) failed
MPID_Comm_disconnect(493)......:
MPIR_Comm_free_impl(809).......:
MPIR_Comm_delete_internal(1224): Communicator (handle=84000003) being freed has 1 unmatched message(s)
Abort(408505615) on node 2 (rank 2 in comm 0): application called MPI_Abort(MPI_COMM_WORLD, 408505615) - process 2
error : MPI driver : Other MPI error, error stack:
internal_Comm_disconnect(81)...: MPI_Comm_disconnect(comm=0x55e844d37028) failed
MPID_Comm_disconnect(493)......:
MPIR_Comm_free_impl(809).......:
MPIR_Comm_delete_internal(1224): Communicator (handle=84000003) being freed has 1 unmatched message(s)
Abort(542723343) on node 3 (rank 3 in comm 0): application called MPI_Abort(MPI_COMM_WORLD, 542723343) - process 3
error : MPI driver : Other MPI error, error stack:
internal_Comm_disconnect(81)...: MPI_Comm_disconnect(comm=0x55935d30f028) failed
MPID_Comm_disconnect(493)......:
MPIR_Comm_free_impl(809).......:
MPIR_Comm_delete_internal(1224): Communicator (handle=84000003) being freed has 1 unmatched message(s)
Abort(207179023) on node 4 (rank 4 in comm 0): application called MPI_Abort(MPI_COMM_WORLD, 207179023) - process 4
error : MPI driver : Other MPI error, error stack:
internal_Comm_disconnect(81)...: MPI_Comm_disconnect(comm=0x5585f7750028) failed
MPID_Comm_disconnect(493)......:
MPIR_Comm_free_impl(809).......:
MPIR_Comm_delete_internal(1224): Communicator (handle=84000003) being freed has 1 unmatched message(s)
Abort(744049935) on node 5 (rank 5 in comm 0): application called MPI_Abort(MPI_COMM_WORLD, 744049935) - process 5

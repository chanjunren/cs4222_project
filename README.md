Instructions to install

1. Clone the entire repository to contiki/examples, then go to the repository.
2. Commands to compile: <br/>
`make TARGET=srf06-cc26xx BOARD=sensortag/cc2650 trace_together_quorum.bin CPU_FAMILY=cc26xx`
3. Commands to compile for Cooja sky file:<br/>
`make TARGET=sky trace_together_quorum.upload`
4. Also, there is an existing Cooja sky file (trace_together_quorum.sky) and configuration file (trace_together_quorum.csc) we built that can be used for Cooja simulation.
 
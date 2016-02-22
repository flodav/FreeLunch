# The Free-Lunch lock profiler

Free-Lunch is a lock profiler whose goal is to identify the locks that
most impede thread progress. It is designed around 3 key concepts:

- Critical Section Pressure (CSP): CSP is a metric that measures the
  impact of locks on overall thread progress. The more the CSP is
  important, the more locks are preventing threads (and thus the
  application) from making progress,

- Regular reports of lock contention: even transient lock contention
  can hurt application performance, therefore Free-Lunch regularly
  computes and reports CSP throughout the application to detect lock
  issues as sooon as it arises,

- Low overhead implementation: even the best developpers can't detect
  all performance bugs prior to real-world deployment. Free-Lunch
  overhead remains low (< 6% according to our evaluation), which makes
  it suitable for a continuous usage without degrading user
  experience.


Free-Lunch is implemented directly inside the Hotspot JVM from
[OpenJDK
8](http://www.java.net/download/openjdk/jdk8/promoted/b132/openjdk-8-src-b132-03_mar_2014.zip).
To use it, you have to run your application normally with the provided
JVM and Free-Lunch will start profiling the application automatically.


For more information about Free-Lunch (design, implementation and
evaluation), download the paper [Continuously measuring critical
section pressure with the Free-Lunch
profiler](http://dl.acm.org/citation.cfm?id=2660210&CFID=575763959&CFTOKEN=77214719)
(freely available from this
[webpage](http://2014.splashcon.org/track/oopsla2014#program)) or the
thesis [Continuous and Efficient Lock Profiling for Java on Multicore
Architectures](https://hal.inria.fr/tel-01263203) associated with this
research work.



## Installation

Download either binaries compiled for Linux x64 architectures or
compile it from source.

### Using precompiled binaries

1. Download binaries from the [latest release](https://github.com/flodav/FreeLunch/releases/download/259ea53f/freelunch-linux-amd64.tar.gz).
2. Extract the archive.
3. The `java` binary is located under `bin/` directory.


### Configuration and compilation from source

1. Clone the project with `git clone
https://github.com/flodav/FreeLunch.git` or [download
it](https://github.com/flodav/FreeLunch/archive/master.zip).
2. Run `make config-release` from the root directory to configure the
project. Install any missing dependency if necessary.
3. Run `make release` to compile the source code.
4. The `java` binary is located under
`freelunch/build/linux-x86_64-normal-server-release/jdk/bin/`
directory.

This will build the optimized version of the JVM. To build a debug
version, replace the word _release_ with either _fastdebug_ for the
fastdebug build or _slowdebug_ for the slow debug build.

If you intend to use the JVM with GDB, uncomment the
`--disable-zip-debug-info` option in the Makefile to include debugging
information.


## Usage

Launch your application with the java binary:

`java myapplication`

FreeLunch then creates a file named _monitor\_contention.PID_ and
starts recording information about lock contention throughout the
application.


## Output

Unless otherwise specified, Free-Lunch use the following command-line
options:

`java -XX:MinimumTimeBetweenTwoPhases=1000
-XX:CSPThresholdSummary=0.01 -XX:KContentedLocksPhase=1
-XX:StackFramesDisplayedCount=10 -XX:+PrintLockCSPSummary
-XX:+PrintLockStackTraceSummary`

Let's study the output from the Xalan application (an XSLT processor
for transforming XML documents into HTML) from [the Dacapo benchmarks
suite](http://dacapobench.org/download.html).

```
*****  Elapsed time: 1034 ms. | Phase average CSP: 7.35%
 1 - Phase CSP: 4.61 %, Class: org.dacapo.harness.DacapoClassLoader  (0 total frames)
*****  Elapsed time: 2308 ms. | Phase average CSP: 5.86%
 1 - Phase CSP: 2.81 %, Class: org.apache.xml.utils.XMLReaderManager  (7 total frames)
	at org.apache.xml.utils.XMLReaderManager.getXMLReader(XMLReaderManager.java:84)
	at org.apache.xml.dtm.ref.DTMManagerDefault.getXMLReader(DTMManagerDefault.java:610)
	at org.apache.xml.dtm.ref.DTMManagerDefault.getDTM(DTMManagerDefault.java:282)
	at org.apache.xalan.transformer.TransformerImpl.transform(TransformerImpl.java:699)
	at org.apache.xalan.transformer.TransformerImpl.transform(TransformerImpl.java:1273)
	at org.apache.xalan.transformer.TransformerImpl.transform(TransformerImpl.java:1251)
	at org.dacapo.xalan.XSLTBench$XalanWorker.run(XSLTBench.java:102)
*****  Elapsed time: 3403 ms. | Phase average CSP: 9.29%
 1 - Phase CSP: 2.71 %, Class: org.apache.xpath.axes.IteratorPool  (23 total frames)
	at org.apache.xpath.axes.IteratorPool.getInstance(IteratorPool.java:88)
	at org.apache.xpath.axes.LocPathIterator.asIterator(LocPathIterator.java:267)
	at org.apache.xalan.templates.ElemApplyTemplates.transformSelectedNodes(ElemApplyTemplates.java:207)
	at org.apache.xalan.templates.ElemApplyTemplates.execute(ElemApplyTemplates.java:178)
	at org.apache.xalan.transformer.TransformerImpl.executeChildTemplates(TransformerImpl.java:2400)
	at org.apache.xalan.templates.ElemLiteralResult.execute(ElemLiteralResult.java:1376)
	at org.apache.xalan.templates.ElemApplyTemplates.transformSelectedNodes(ElemApplyTemplates.java:395)
	at org.apache.xalan.templates.ElemApplyTemplates.execute(ElemApplyTemplates.java:178)
	at org.apache.xalan.transformer.TransformerImpl.executeChildTemplates(TransformerImpl.java:2400)
	at org.apache.xalan.templates.ElemLiteralResult.execute(ElemLiteralResult.java:1376)
*****  Elapsed time: 4434 ms. | Phase average CSP: 28.97%
 1 - Phase CSP: 12.91 %, Class: java.util.Hashtable  (23 total frames)
	at java.util.Hashtable.get(Hashtable.java:362)
	...
...
-----------------------------------------------------
Monitors ranked by average CSP (Threshold: 0.01 %)
Rank   Average CSP         Object class
   1     19.03 %     java.util.Hashtable
   2      1.54 %     org.apache.xpath.axes.IteratorPool
   3      0.55 %     org.dacapo.harness.DacapoClassLoader
   4      0.43 %     sun.net.www.protocol.jar.URLJarFile
   5      0.35 %     org.apache.xml.utils.XMLReaderManager
   6      0.19 %     org.apache.xpath.axes.IteratorPool
   7      0.07 %     sun.net.www.protocol.jar.JarFileFactory
   8      0.07 %     java.lang.Class
   9      0.06 %     org.apache.xpath.axes.IteratorPool
...
-----------------------------------------------------
Monitors ranked by average CSP (Threshold: 0.01 %)
Displaying the associated stack trace (10 frame(s) displayed maximum)
Rank   Average CSP         Object class
   1     19.03 %     java.util.Hashtable     (23 frame(s) available)
	at java.util.Hashtable.get(Hashtable.java:362)
	at org.apache.xalan.templates.TemplateList.getTemplateFast(TemplateList.java:508)
	at org.apache.xalan.templates.ElemApplyTemplates.transformSelectedNodes(ElemApplyTemplates.java:296)
	at org.apache.xalan.templates.ElemApplyTemplates.execute(ElemApplyTemplates.java:178)
	at org.apache.xalan.transformer.TransformerImpl.executeChildTemplates(TransformerImpl.java:2400)
	at org.apache.xalan.templates.ElemLiteralResult.execute(ElemLiteralResult.java:1376)
	at org.apache.xalan.templates.ElemApplyTemplates.transformSelectedNodes(ElemApplyTemplates.java:395)
	at org.apache.xalan.templates.ElemApplyTemplates.execute(ElemApplyTemplates.java:178)
	at org.apache.xalan.transformer.TransformerImpl.executeChildTemplates(TransformerImpl.java:2400)
	at org.apache.xalan.templates.ElemLiteralResult.execute(ElemLiteralResult.java:1376)
   2      1.54 %     org.apache.xpath.axes.IteratorPool     (23 frame(s) available)
	at org.apache.xpath.axes.IteratorPool.getInstance(IteratorPool.java:88)
	at org.apache.xpath.axes.LocPathIterator.asIterator(LocPathIterator.java:267)
	at org.apache.xalan.templates.ElemApplyTemplates.transformSelectedNodes(ElemApplyTemplates.java:207)
	at org.apache.xalan.templates.ElemApplyTemplates.execute(ElemApplyTemplates.java:178)
	at org.apache.xalan.transformer.TransformerImpl.executeChildTemplates(TransformerImpl.java:2400)
	at org.apache.xalan.templates.ElemLiteralResult.execute(ElemLiteralResult.java:1376)
	at org.apache.xalan.templates.ElemApplyTemplates.transformSelectedNodes(ElemApplyTemplates.java:395)
	at org.apache.xalan.templates.ElemApplyTemplates.execute(ElemApplyTemplates.java:178)
	at org.apache.xalan.transformer.TransformerImpl.executeChildTemplates(TransformerImpl.java:2400)
	at org.apache.xalan.templates.ElemLiteralResult.execute(ElemLiteralResult.java:1376)
   3      0.55 %     org.dacapo.harness.DacapoClassLoader     (0 frame(s) available)
   	...
...
```

This output is divided in 3 parts separated by "-----" lines. The
first part displays the average CSP and the topmost contented locks
for the current phase. It is printed regularly throughout the
application. The second shows a summary of locks having an average CSP
superior to a specified threshold and the third part also displays a
the same information with in addition a stack trace in order to locate
the lock.

The first part shows the time elapsed from the start of the
application and the average CSP for the current phase. The duration of
the phase can be computed by substracting the elapsed time of the
phase by the one of the previous phase. Additionnaly, the CSP during
the phase of the topmost contented lock(s) along with their stack
trace is displayed. The number of locks is set with the
`KContentedLocksPhase` option.

The second part outputs every monitors with the class of the
associated Java object, ranked by average CSP over the application
length and superior to a threshold specified by the
CSPThresholdSummary option (0.01% by default).

The last part displays the same information as in the second part
along with the stack trace pointing to the location of the lock in the
source code.



## Free-Lunch options

List of all the options available in Free-Lunch with the default value in parenthesis:

* EnableRegularSafepoint (0): Enable safepoints at regular intervals
  (in ms.). Disabled by default.

* MinimumTimeBetweenTwoPhases (1000): Minimum time between 2 phases
  (in ms.). Used to avoid too many safepoints during a small amount of
  time.

* CSPThreshold (0): Print the current phase CSP only if it is above
  the provided value. Disabled by default.

* CSPForClass (NULL): Compute the CSP only for locks of the specified
  class. For instance: java.util.Hashtable . Disabled by default.

* CSPThresholdSummary (0.01): Do not print locks for which the CSP is
  below the provided value (in %).

* KContentedLocksPhase (1): Show the K most contented locks for each
  phase.

* StackFramesDisplayedCount (10): The number of frames printed for
  each stack trace. Maximum is set to 80. Change the MAX\_STACK\_DEPTH
  macro in macros.hpp to display more.

* PrintLockCSPSummary (true): Print a summary of the average CSP for
  every lock of the application.

* PrintLockStackTraceSummary (true): Print a summary of the average
  CSP for every lock of the application along with a stack trace to
  locate the lock.

* PrintLockingFrequencyStat (false): Print the global locking and
  calls to Object.wait() frequency. COUNT\_LOCKED and COUNT\_WAITED
  macros must be enabled (file macros.hpp) to work.


## License

© 2011-2016 Florian David
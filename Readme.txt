

0       7 8     15 16    23 24     31 
+--------+--------+--------+--------+
|            stype size             |
+--------+--------+--------+--------+
|   s        t         y       p    | #<--styp has same defintion as ftyp
+--------+--------+--------+--------+
|            major brand            | #<--mp41
+--------+--------+--------+--------+
|           minor version           | #<--0
+--------+--------+--------+--------+
|         compatible brands         | #<--mp41
+--------+--------+--------+--------+
|         compatible brands         | #<--lseg
+--------+--------+--------+--------+
|             emsg size             |
+--------+--------+--------+--------+
|   e        m         s       g    |
+--------+--------+--------+--------+
|   ver  |           flags          | #<--ver = 1, flags = 0x00
+--------+--------+--------+--------+
|            timescale              | #<--from SegmentTemplate in audio AdaptSet
+--------+--------+--------+--------+
|         presentation_time         | #<--8 bytes, copy tfdt from segment
|                                   |
+--------+--------+--------+--------+
|          event_duration           | #<--0xFFFFFFFF
+--------+--------+--------+--------+
|                id                 | #<--random number
+--------+--------+--------+--------+
|   u         r        n       :    | #<--scheme_id_uri copied from DASH spec
|   m         p        e       g    |
|   :         d        a       s    |
|   h         :        e       v    |
|   e         n        t       :    |
|   2         0        1       2    |
|  \0                               | #<--FIXME: it is not 32 bit word aligned
+--------+--------+--------+--------+
|      value      | messsage_data   | #<--value = 0x3300
+--------+--------+--------+--------+
|         message_data con't        | #<--bytes of xml equal to
|                ...                | #<--emsg size - other fields
+--------+--------+--------+--------+
|             moof size             | 
+--------+--------+--------+--------+
|   m        o         o       f    |
+--------+--------+--------+--------+
|                ...                | #<--rest of the audio segment
+--------+--------+--------+--------+




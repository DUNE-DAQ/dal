package dal_test;

import java.util.Arrays;
import java.util.Comparator;

public class TestOks {

  public static void main(String args[]) {

    if(args.length != 1) {
      ers.Logger.error(new ers.Issue("database argument is required"));
      System.exit(1);
   }

    try {

       // create configuration object
       config.Configuration db = new config.Configuration(args[0]);

       // get application object by it's identity (known in advance) and print out it's name
       dal.OnlineSegment seg = dal.OnlineSegment_Helper.get(db, "setup");

       if(seg == null) {
         ers.Logger.error(new ers.Issue("can not find online segment \'setup\'"));
         System.exit(1);
       }


       // get a partition object (if exists) and use generated method to print out it's contents
       dal.Partition part_objs[] = dal.Partition_Helper.get(db, new config.Query());

       if(part_objs.length == 0) {
         ers.Logger.error(new ers.Issue("can not find a partition object"));
         System.exit(1);
       }

       Arrays.sort(part_objs, (dal.Partition o1, dal.Partition o2) -> {
           return o1.UID().compareTo(o2.UID());
       });

       for(int j = 0; j < part_objs.length; ++j) { part_objs[j].print(""); System.out.println(""); }
    }

    catch(final config.ConfigException ex) {
      ers.Logger.error(ex);
      System.exit( 1 );
    }

  }

}

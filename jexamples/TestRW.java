package dal_test;

public class TestRW {
  
  public static void test_application(dal.Application a) {
    try {
      System.out.println(" * test object " + a.UID() + '@' + a.class_name());
      
      System.out.println("   - valid:   " + a.config_object().is_valid());
      System.out.println("   - created: " + a.config_object().is_created());
      System.out.println("   - updated: " + a.config_object().is_updated());
      System.out.println("   - timeout: " + a.get_InitTimeout());
    }
    catch(config.ConfigException ex) {
      ers.Logger.error(ex);
    }
  }

  public static void main(String args[]) {

    if(args.length != 3) {
      ers.Logger.error(new ers.Issue("wrong number of agrument\n" + "usage: java dal_test.TestRW \'technology[:server-name]\' \'new-database-file-name-1\' \'new-database-file-name-2\'"));
      System.exit(1);
    }


    try {

         // create configuration object

       config.Configuration db = new config.Configuration(args[0]);


         // create new database file

       String[] includes = new String[1];
       includes[0] = "daq/schema/core.schema.xml";
       db.create(args[1], includes);
       db.create(args[2], includes);


         // create segment object

       dal.Segment s1 = dal.Segment_Helper.create(db, args[1], "S1.1");


         // create segment's controller

       dal.RunControlApplication ctrl1 = dal.RunControlApplication_Helper.create(db, s1, "Ctrl1.1");
       ctrl1.set_Parameters("-n S1.1 -b /tmp/s1.1/backup");
       ctrl1.set_RestartParameters("-n S1.1 -b /tmp/s1.1/backup -r");
       ctrl1.set_InitTimeout(22);
       s1.set_IsControlledBy(ctrl1);


         // create segment's applications

       dal.CustomLifetimeApplication[] apps = new dal.CustomLifetimeApplication[2];

       apps[0] = dal.CustomLifetimeApplication_Helper.create(db, s1, "App0");
       apps[1] = dal.CustomLifetimeApplication_Helper.create(db, s1, "App1");

       s1.set_Applications(apps);


         // create SW_Reposioty and Programs

       dal.SW_Repository swr = dal.SW_Repository_Helper.create(db, s1, "SW Repos");
       dal.Tag[] tags = new dal.Tag[1];
       tags[0] = dal.Tag_Helper.create(db, s1, "a tag");
       swr.set_Tags(tags);

       dal.Binary[] binaries = new dal.Binary[2];
       binaries[0] = dal.Binary_Helper.create(db, s1, "Program.Ctrl.1");
       binaries[1] = dal.Binary_Helper.create(db, s1, "Program.App.1");

       binaries[0].set_BelongsTo(swr);
       binaries[1].set_BelongsTo(swr);
       swr.set_SW_Objects(binaries);

       ctrl1.set_Program(binaries[0]);
       apps[0].set_Program(binaries[1]);
       apps[1].set_Program(binaries[1]);


         // print modified files

       String[] modified = db.get_updated_dbs();

       if(modified != null) {
         System.out.println("There are " + modified.length + " updated files");

         for ( int i = 0; i < modified.length; i++ ) {
           System.out.println(" * \"" + modified[i] + "\"");
         }
       }

       
         // create and move an object
       
       dal.CustomLifetimeApplication an_app = dal.CustomLifetimeApplication_Helper.create(db, s1, "App-X");
       an_app.set_Program(binaries[1]);
       db.move(an_app.config_object(), args[2]);

       dal.CustomLifetimeApplication app11 = dal.CustomLifetimeApplication_Helper.create(db, args[2], "App-11");
       app11.set_Program(binaries[1]);
       app11.set_InitTimeout(11);

       dal.RunControlApplication rc_app14 = dal.RunControlApplication_Helper.create(db, args[2], "App-14");
       rc_app14.set_Program(binaries[1]);
       rc_app14.set_InitTimeout(14);


         // commit changes

       db.commit("dal/jexamples/TestRW.java");


         // run test for unread and abort operations

       System.out.println("Create RunControlApplication App-12");

       dal.RunControlApplication rc_app12 = dal.RunControlApplication_Helper.create(db, args[2], "App-12");
       rc_app12.set_Program(binaries[1]);
       rc_app12.set_InitTimeout(12);

       System.out.println("Create RunControlApplication App-13");

       dal.RunControlApplication rc_app13 = dal.RunControlApplication_Helper.create(db, args[2], "App-13");
       rc_app13.set_Program(binaries[1]);
       rc_app13.set_InitTimeout(13);

       System.out.println("Create Application App-12");
       dal.Application app12 = dal.Application_Helper.cast(rc_app12);
       System.out.println("Create Application App-13");
       dal.Application app13 = dal.Application_Helper.cast(rc_app13);
       System.out.println("Create Application App-14");
       dal.Application app14 = dal.Application_Helper.cast(rc_app14);
 
       System.out.println("Destroy Application RunControlApplication14");

       db.destroy(rc_app14.config_object());

       System.out.println("Test objects after creation:");
       test_application(an_app);
       test_application(app11);
       test_application(app12);
       test_application(app13);
       test_application(app14);
       test_application(rc_app12);
       test_application(rc_app13);
       test_application(rc_app14);


         // unread objects
       
       db.unread_all_objects(true);
       
       System.out.println("Test objects after unread_all_objects(true):");
       test_application(an_app);
       test_application(app11);
       test_application(app12);
       test_application(app13);
       test_application(app14);
       test_application(rc_app12);
       test_application(rc_app13);
       test_application(rc_app14);

       app11.set_InitTimeout(111);

         // abort changes

       db.abort();
       System.out.println("Test objects after abort():");
       test_application(an_app);
       test_application(app11);
       test_application(app12);
       test_application(app13);
       test_application(app14);
       test_application(rc_app12);
       test_application(rc_app13);
       test_application(rc_app14);


         // close database

       db.unload();
    }
    catch(final config.ConfigException ex) {
      ers.Logger.error(ex);
      System.exit(2);
    }
  }
}

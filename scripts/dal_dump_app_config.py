#!/usr/bin/env python

class TableRow :
    def __init__(self, fill, separator, s1, s2, s3, s4, s5, s6):
        self.fill = fill
        self.separator = separator
        self.items = []
        if s1 is not None :
            self.items.append(s1)
            self.items.append(s2)
            self.items.append(s3)
            self.items.append(s4)
            self.items.append(s5)         
            self.items.append(s6)  
        else :
            self.items.append("=")
            self.items.append("=") 
            self.items.append("=") 
            self.items.append("=") 
            self.items.append("=") 
            self.items.append("=") 
                         
    @classmethod
    def fromhost(cls, fill, separator, host ):
        return cls(fill, separator, "", "", host.id + "@" + host.class_name , "", "", "")
    @classmethod
    def fromobjects(cls, fill, separator, num, app, host, seg, seg_id, app_id):
        return cls(fill, separator, num, app.id + "@" + app.class_name, host.id + "@" + host.class_name, seg.id + "@" + seg.class_name, seg_id, app_id )
    @classmethod
    def fromchar(cls, fill, separator):
        return cls(fill, separator, None,None,None,None,None,None)

  
if __name__ == '__main__':

    import argparse
    import sys    
     
    import oksdbinterfaces

    import dal

        
    parser = argparse.ArgumentParser(description='''This program prints out applications and their configuration parameters using Partition::get_all_applications() algorithm.
        Usage: dal_dump_app_config.py [-d database-name] -p name-of-partition [-t [types ...]] [-c [ids ...]] [-s [ids ...]]''')
    parser.add_argument("-d", "--database_name", 
                        help="name of the database (ignore TDAQ_DB variable)")
    parser.add_argument("-p", "--partition_name", required = True,
                        help="name of the partition object")
    parser.add_argument("-t", "--application_types",   nargs = '*',
                        help="filter out all applications except given classes (and their subclasses)")
    parser.add_argument("-c", "--hosts",   nargs = '*',
                        help="filter out all applications except those which run on given hosts")
    parser.add_argument("-s", "--segments", nargs = '*',
                        help="filter out all applications except those which belong to given segments")  
    parser.add_argument("-b", "--show_backup_hosts", action='count',
                        help="print backup hosts")                       

    args = parser.parse_args()

      # Open test database
    db = oksdbinterfaces.Configuration(args.database_name)

    # get Partition object
    partition = db.get_dal('Partition', args.partition_name)

    app_config_list = partition.get_all_applications(db, args.application_types , args.segments, args.hosts)

    print ('Got ' + str(len(app_config_list)) + ' applications:')

    rows = []
    rows.append(TableRow.fromchar('=', '='))
    rows.append(TableRow(' ', '|', "num","Application Object","Host","Segment","Segment unique id","Application unique id"))    
    rows.append(TableRow.fromchar('=', '='))    

    count = 1

    for i in app_config_list :
#         init = i.get_initialization_depends_from(app_config_list)
#         print "initialisation of",i.get_app_id(),"depends from",len(init)
#         shut = i.get_shutdown_depends_from(app_config_list)
#         for x in init:
#             print '   ',x.get_app_id(),'on',x.get_host().id
#         print "shutdown of",i.get_app_id(),"depends from",len(shut)
#         for x in shut:
#             print '   ',x.get_app_id(),'on',x.get_host().id
        rows.append(TableRow.fromobjects(' ', '|', str(count), i.get_base_app(), i.get_host(), i.get_base_seg(), i.get_seg_id(), i.get_app_id()))         
        count+=1
        if args.show_backup_hosts is not None:
            backup_hosts = i.get_backup_hosts()
            for j in backup_hosts :
                rows.append(TableRow.fromhost(' ', '|', j))  

    rows.append(TableRow.fromchar('=', '=')) 

    cw = [1,1,1,1,1,1]

    for row in rows:
        #print row.items
        for idx in range(0,6) : 
            length = len(str(row.items[idx]))
            if length > cw[idx]:
                cw[idx] = length
        #print cw

    align_left = [False, True, True, True, True, True]

    for row in rows :
        line =[]
        for idx in range(0,6):
            line.append(row.separator)
            line.append(row.fill)
            if align_left[idx]:
                line.append(row.items[idx])
                line.append(row.fill*(cw[idx]-len(row.items[idx]))) 
                line.append(row.fill)
            else :
                line.append(row.fill*(cw[idx]-len(row.items[idx])))        
                line.append(row.items[idx])        
                line.append(row.fill)

        line.append(row.separator)     
        line = ''.join(str(e) for e in line)  
        print(line)
                               


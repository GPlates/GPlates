#Step 1: You need to import gplates.py. Put gplates.py in python path.
import gplates
from types import *

def main():
    try:
        #Step 2: Create a client like this.
        c = gplates.Client('localhost', 9777)
    except:
        print 'Failed to connect to GPlates'
        return

    coreg_layer = c.get_coregistration_layer()
    b,e,i = coreg_layer.get_time_setting()
    print b, e, i
    #Step 3: Here is how you can get all feature ids of coregistration "seeds".
    #The get_coreg_seeds() function returns a list of string.
    seeds = coreg_layer.get_coreg_seeds()
    if seeds:
        for seed in seeds:
            #Step 4: Give get_begin_time() a feature id,
            #it will return you the begin time of the feature as a float.
            #If the time is "distant past", the return value is "inf".
            print seed + '(' + str(coreg_layer.get_begin_time(seed)) +')'

    #Step 5: Get association definitions.
    #The get_coreg_associations() returns a list of Association objects.
    associations = coreg_layer.get_coreg_associations()
    if associations:
        for a in associations:
            asso_data = (
            #From Association object you can get the following data as a string.
            str(a.get_target_name()) + '\t' +
            str(a.get_association_type()) + '\t' +
            str(a.get_attribute_name())+ '\t' +
            str(a.get_attribute_type())+ '\t' +
            str(a.get_data_operator()) + '\t' )
            ps = a.get_association_parameters()
            if ps:
                for p in ps:
                    asso_data += str(p) + '\t'
            print asso_data

    #Given a reconstruction time, the get_coreg_data() returns a table of coregistration data.
    #THe data might be string or Decimal depends on its nature.
    t = coreg_layer.get_coreg_data(20)
    if t:
        for row in t:
            row_data = ''
            for cell in row:
                row_data += str(cell) + '\t'
                #print cell
                #print type(cell)
            print row_data

if __name__ == '__main__':
    main()    

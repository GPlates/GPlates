import socket
from xml.dom.minidom import parseString
from decimal import *

class Association:
    def __init__(self, xml_str):
        dom = parseString(xml_str)

        element_N = dom.getElementsByTagName('Name')
        if element_N:
            self.assoc_name = element_N[0].firstChild.nodeValue
        else:
            print 'No Name element found.'
            raise

        element_LN = dom.getElementsByTagName('LayerName')
        if element_LN:
            self.target_name = element_LN[0].firstChild.nodeValue
        else:
            print 'No LayerName element found.'
            raise
        
        element_AT = dom.getElementsByTagName('AssociationType')
        if element_AT:
            self.type = element_AT[0].firstChild.nodeValue
        else:
            print 'No AssociationType element found.'
            raise

        element_AN = dom.getElementsByTagName('AttributeName')
        if element_AN:
            self.attr_name = element_AN[0].firstChild.nodeValue
        else:
            print 'No AttributeName element found.'
            raise
        
        element_AttrType = dom.getElementsByTagName('AttributeType')
        if element_AttrType:
            self.attr_type = element_AttrType[0].firstChild.nodeValue
        else:
            print 'No AttributeType element found'
            raise
        
        element_DO = dom.getElementsByTagName('DataOperator')
        if element_DO:
            self.data_operator = element_DO[0].firstChild.nodeValue
        else:
            print 'No DataOperator element found.'
            
        element_AP = dom.getElementsByTagName('AssociationParameter')
        self.assn_paras = []
        if element_AP:
            for e in element_AP:
                self.assn_paras.append(e.firstChild.nodeValue)
        

    def get_name(self):
        return self.assoc_name
    
    def get_target_name(self):
        return self.target_name

    def get_association_type(self):
        return self.type

    def get_association_parameters(self):
        return self.assn_paras

    def get_attribute_name(self):
        return self.attr_name

    def get_attribute_type(self):
        return self.attr_type

    def get_data_operator(self):
        return self.data_operator


class CoregistrationLayer:
    def __init__(self, coreg_layer_name=None):
        self.layer_name = coreg_layer_name
       

    #The get_coreg_seeds returns all feature ids of coregistration "seeds" as a list of strings.
    def get_coreg_seeds(self):
        request=('<Request><Name>GetSeeds</Name><LayerName>'
                 + self.layer_name +'</LayerName></Request>')
        c = Client()
        try:
            data = c.send_request_return_response(request)
            dom = parseString(data)
            ele = dom.getElementsByTagName('Response')
            if ele:
                fc = ele[0].firstChild
                if fc:
                    return fc.nodeValue.split()
            raise
              
        except:
            print c.get_error_msg()
            print "No Seed found."
            return []


    #Given  a feature id,
    #the get_begin_time() returns the begin time of the feature as a float.
    #If the time is "distant past", the return value is "inf".
    def get_begin_time(self, feature_id):
        request=('<Request><Name>GetBeginTime</Name><FeatureID>'
                                + feature_id + '</FeatureID></Request>')
        ret = 0
        c = Client()
        try:
            data = c.send_request_return_response(request)
                
            dom = parseString(data)
            ele = dom.getElementsByTagName('Response')
            if ele:
                ret = float(ele[0].firstChild.nodeValue)     
        except:
            print c.get_error_msg()
            raise
        return ret


    #The get_coreg_associations() returns a list of Association objects.
    def get_coreg_associations(self):
        request=('<Request><Name>GetAssociations</Name><LayerName>'
                 + self.layer_name +'</LayerName></Request>')
        ret = []
        data = ''
        self.d_client = Client()
        try:
            data = self.d_client.send_request_return_response(request)
            dom = parseString(data)
            for e in dom.getElementsByTagName('Assosiation'):
                ret.append(Association(e.toxml()))
        except:
            print 'Failed to get coregistration associations.'
            print self.d_client.get_error_msg()
            print data
            raise
            
        return ret

    def get_time_setting(self):
        request=('<Request><Name>GetTimeSetting</Name></Request>')
        data = ''
        self.d_client = Client()
        try:
            bt = 140.0
            et = 0.0
            inc = 1.0
            data = self.d_client.send_request_return_response(request)
            dom = parseString(data)

            bt_element = dom.getElementsByTagName('BeginTime')
            if bt_element:
                bt = float(bt_element[0].firstChild.nodeValue)

            et_element = dom.getElementsByTagName('EndTime')
            if et_element:
                et = float(et_element[0].firstChild.nodeValue)

            inc_element = dom.getElementsByTagName('Increment')
            if inc_element:
                inc = float(inc_element[0].firstChild.nodeValue)
        except:
            print 'Failed to get time setting.'
            print self.d_client.get_error_msg()
            print data
             
        return bt, et, inc

    #Given a reconstruction time, the get_coreg_data() returns a table of coregistration data.
    #THe data might be string or float depends on its nature.    
    def get_coreg_data(self, time):
        request=('<Request><Name>GetAssociationData</Name><ReconstructionTime>'
                + str(time) + '</ReconstructionTime><LayerName>'
                 + self.layer_name +'</LayerName></Request>')
        data = ''
        self.d_client = Client()
        try:
            data = self.d_client.send_request_return_response(request)
            dom = parseString(data)
        except:
            print self.d_client.get_error_msg()
            print data
            raise
        
        ele = dom.getElementsByTagName('DataTable')
        if ele:
            row=0
            column=0
            attrs = ele[0].attributes
            for index in range(attrs.length):
                    if attrs.item(index).name == 'row':
                            row = int(attrs.item(index).value)
                    if attrs.item(index).name == 'column':
                            column = int(attrs.item(index).value)
           
            cells = ele[0].getElementsByTagName('c')
            data_list = []
            for c in cells:
                fc = c.firstChild
                if not fc:
                    data_list.append('')
                    continue

                str_val = fc.nodeValue
                               
                if str_val.isdigit():
                    data_list.append(float(str_val))
                else:
                    data_list.append(str_val)
                    
            t = [data_list[i:i+column] for i  in range(0, len(data_list), column)]
            return t
        else:
            print 'Cannot find data.'
            #print data
            return None
        

class Client:
    def __init__(self, host='localhost', port=9777, timeout=300):
        is_inside_gplates = False;
        self.errorMsg = None
        try:
            import pygplates
            if pygplates.is_embedded():
                
                is_inside_gplates = True
        except:
            pass
        
        if is_inside_gplates:
            print 'This script cannot run in GPlates Python Console.'
            raise
            
        self.host = host
        self.port = port
        self.timeout=timeout
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((self.host, self.port))
       
    def get_coregistration_layer(self, layer_name=''):
         return CoregistrationLayer(layer_name)


    def set_reconstruction_time(self, time):
        request=('<Request><Name>SetReconstructionTime</Name><ReconstructionTime>'
                + str(time) + '</ReconstructionTime></Request>')
        data = ''
        try:
            data = self.send_request_return_response(request)
            dom = parseString(data)
            if dom.getElementsByTagName('ErrorMsg'):
                raise
        except:
            print self.get_error_msg()
            print  request          
            return False
        return True


    def send_request_return_response(self, request):
        self.errorMsg = ''
        self.client_socket.sendall(request)
        data = self._recvall()
        if not data:
            print 'Received nothing from socket.'
            raise
        if(data.find('<ErrorMsg>') != -1):
            dom = parseString(data)
            e = dom.getElementsByTagName('ErrorMsg')
            if e:
                self.errorMsg = e[0].firstChild.nodeValue  
            print 'Failed to execute reqeust: ' + request
            
        return data

    def get_error_msg(self):
        return self.errorMsg

    def _recvall(self):
        data=''
        self.client_socket.settimeout(self.timeout)
        while True:
            d = self.client_socket.recv(1024)
            data += d
            if '</Response>' in d:
                break
        return data

    def __del__(self):
        self.client_socket.close()



if __name__ == "__main__":
    c = Client('localhost', 9777)
    coreg_layer = c.get_coregistration_layer()
    
    seeds = coreg_layer.get_coreg_seeds()
    for seed in seeds:
            print seed
            print coreg_layer.get_begin_time(seed)
            
    for a in coreg_layer.get_coreg_associations():
        print a.get_target_name()
        print a.get_association_type()
        ps = a.get_association_parameters()
        if ps:
            for p in ps:
                print p
        print a.get_attribute_name()
        print a.get_attribute_type()
        print a.get_data_operator()
    
    t = coreg_layer.get_coreg_data(20)
    for row in t:
            for cell in row:
                    print cell 
    
            


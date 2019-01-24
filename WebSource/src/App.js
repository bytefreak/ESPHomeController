import React, { Component } from 'react';
//import logo from './logo.svg';
import './App.css';
import Menu from "./Components/Menu";
import { BrowserRouter as Router, Route, Link } from 'react-router-dom';
import Popup from "reactjs-popup";
import BurgerIcon from "./Components/BurgerIcon";
import Relay from "./Components/Relay";
import Services from "./Components/Services";
import LDR from "./Components/LDR"
import { getHomeUrl, getConfigData, getBaseuri, doFetch } from "./Components/utils"

import Triggers from './Components/Triggers';
import RGBStrip from './Components/RGBStrip';
import TimeCtl from "./Components/TimeCtl"

const styles = {
    fontFamily: "sans-serif",
    textAlign: "center",
    marginTop: "0px"
};
const contentStyle = {
    background: "rgba(255,255,255,0)",
    width: "80%",
    border: "none"
};
/*
const Home = () => (
    <div>
        <h2>Home Page </h2>
    </div>
);
*/



class App extends Component {
    constructor(props) {
        super(props)
        this.state = { services: [] ,triggers:[]}
    }
    componentDidMount() {
        doFetch(getBaseuri() + "/get_info", (data) => { this.setHeaderdata(data) });
        doFetch(getBaseuri() + "/services.json", (data) => { console.log(data); this.setState({ services: data }); });
        doFetch(getBaseuri() + "/triggers.json", (data) => { console.log(data); this.setState({ triggers: data }); });
    }
    componentWillMount() {
        //var obj = JSON.parse('[{"service":"RelayController","name":"Relay","enabled":true,"interval":100,"pin":5},{"service":"TimeController","name":"Time","enabled":true,"interval":1000,"timeoffs":7200,"dayloffs":3600,"server":"pool.ntp.org"},{"service":"RGBStripController","name":"RGBStrip","enabled":true,"interval":1,"pin":23,"numleds":8},{"service":"LDRController","name":"LDR","enabled":true,"interval":1000,"pin":0}]');
        //var obj1 = JSON.parse('[{"type":"TimeToRGBStrip","source":"Time","destination":"Relay","value":[{"isOn":true,"isLdr":true,"time":0,"bg":1,"wxmode":-1}]}]');
       // this.setState({ services : obj });
        //this.setState({ triggers : obj1 });
       
    }
    setHeaderdata(data) {
        document.getElementById("id_ver").innerText = "ver:" + data.version;
        document.getElementById("id_host").innerText = data.hostname;
        document.getElementById("id_async").innerText = "async:"+data.async;
        
    }
    renderService(svc) {
        //console.log("render service");
        //console.log(svc.service);
        switch (svc.service) {
            case "RelayController":
                return (props => <Relay  {...props} compprops={svc} />);
            case "RGBStripController":
                return (props => <RGBStrip {...props} compprops={svc} />);
            case "LDRController":
                return (props => <LDR {...props} compprops={svc} />);
            case "TimeController":
                return (props => <TimeCtl {...props} compprops={svc} />);
            default:
                return (props =><div></div>);
                break;
        }
        return (param=>'Undefined service');
    }

    render() {
        const compprops = {};

        //compprops.servicename = "RelaySwitch"
        //console.debug("AppRender");
       // console.debug(getBaseuri());

    return (
        <div key={"App"} className="App">
       
        
            <div key={"AppRoot"}style={styles}>
           

            <Router>
                    <div key={"RouterRoot"} >
                    <Popup
                        modal
                        overlayStyle={{ background: "rgba(255,255,255,0.98" }}
                        contentStyle={contentStyle}
                        closeOnDocumentClick={false}
                        trigger={open => <BurgerIcon open={open} />}
                        >
                            {close => <Menu close={close} services={this.state.services} />}
                    </Popup>
                         
                        <hr />

                        <Route exact path="/"
                            render={
                                
                                   ()=> this.state.services.map(item =>
                                       //this.renderService(item)
                                       <div key={"RS" + item.name} >

                                               {this.renderService(item)({ compprops:{ item }})}
                                          
                                       </div>
                                    )
                                
                            }
                            
                         />
                  
                   
                        {this.state.services.map(item =>
                            <Route path={"/" + item.name} key={item.service}
                                render={this.renderService(item)}
                            />
                                
                           
                        )}
                        <Route path="/Triggers"
                            render={(props => <Triggers key="triggers" {...props} triggedata={this.state.triggers} servicedata={this.state.services} />)}
                        />
                        <Route path="/Services"
                            render={(props => <Services key="services" {...props} compprops={this.state.services} />)}
                        />
                      
                        
                 
                                          
                </div>
            </Router>
            </div>
        </div>
    );
  }
}

export default App;

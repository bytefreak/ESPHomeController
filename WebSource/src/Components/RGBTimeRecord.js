﻿import React from "react";
import Checkbox from "./Checkbox"
import RangeCtl from "./RangeCtl"
import TimePickCtl from "./TimePickCtl"
import ColorStatus from "./ColorStatus"
import ColorWeel from "./ColorWeel"

class RGBTimeRecord extends React.Component {
    constructor(props) {
        super(props);

        const { handlechange } = props;

        //console.debug("RGBTimeRecord");
       // console.debug(props);
        //this.state = { isOn: false, isLdr: false, time: 0, bg: 150, wxspeed: 1000, color: 0, wxmode: 0 };
        this.handlechange = handlechange;
        this.cs = {};
    };
    onChangeVal(newVal) {
       // this.setState(newVal);
        this.handlechange(newVal);
    }
    render() {

        //console.debug("render rgbtimerecord");
        
        const { item,idx } = this.props;
        
        return (
            <>
                <div className="col s4">
                <TimePickCtl
                    label="Time"
                    timevalue={item.time}
                    handleTimeChange={t => (this.onChangeVal({ time: t.state.timevalue }))}
                    />
                </div>
                <div className="col s2">
                <Checkbox
                    isChecked={item.isOn}
                    label="IsOn"
                    handleCheckboxChange={ch => (this.onChangeVal({ isOn: ch.state.isChecked }))}
                    key="isOn"
                    />
                </div>
                <div className="col s2">
                <Checkbox
                    isChecked={item.isLdr}
                    label="IsLdr"
                    handleCheckboxChange={ch => (this.onChangeVal({ isLdr: ch.state.isChecked }))}
                    key="isLdr"
                    />
                </div>
                <RangeCtl
                    
                    label="Brigthness"
                    rangevalue={item.br}
                    handleRangeChange={br => this.onChangeVal({ bg: br.state.rangevalue })}
                />
                <div className="row">
                    <div className="col s4">
                        <ColorWeel
                            width={200}
                            height={200}
                            onSelect={(intcolor, hexcolor) => { this.onChangeVal({ color: intcolor }) }}
                        />
                    </div>
                    <div className="col s4">
                        <label htmlFor="1">Color</label>
                        <input type="text" value={item.color} name="color"
                            onChange={ev => {
                                var val = parseInt( ev.target.value);
                                this.onChangeVal({ color: val });
                                //this.cs.setBkColor(col);
                            }}
                        />
                    </div>
                    <div className="col s4">
                        <ColorStatus ref={el => this.cs = el} color={item.color}/>
                    </div>
                </div>
            </>
     );
    }
}
export default RGBTimeRecord;